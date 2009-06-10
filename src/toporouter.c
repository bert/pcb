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
  vertex->boxes = NULL;
  vertex->parent = NULL;
  vertex->child = NULL;
  vertex->fakev = NULL;
  vertex->pullx = INFINITY;
  vertex->pully = INFINITY;
  vertex->flags = 0;
  vertex->routingedge = NULL;
  vertex->arc = NULL;
  vertex->oproute = NULL;
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
    if(!strcmp(style->Name, name)) return style->Keepaway + 1.;
  }
  END_LOOP;
  return Settings.Keepaway + 1.;
}

gdouble
lookup_thickness(char *name) 
{
  if(name)
  STYLE_LOOP(PCB);
  {
    if(!strcmp(style->Name, name)) return style->Thick + 1.;
  }
  END_LOOP;
  return Settings.LineThickness + 1.;
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
      if(tv->boxes) {
        pin = (PinType*) TOPOROUTER_BBOX(tv->boxes->data)->data;
        pad = (PadType*) TOPOROUTER_BBOX(tv->boxes->data)->data;

        blue = 0.0f;
        switch(TOPOROUTER_BBOX(tv->boxes->data)->type) {
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

/* TODO: phase out multiple bboxes */
toporouter_bbox_t *
vertex_bbox(toporouter_vertex_t *v) 
{
  GSList *i = v ? v->boxes : NULL;

  if(!i) return NULL;

  if(g_slist_length(i) > 1) {
    printf("WARNING: vertex with multiple bboxes\n");
  }

  return TOPOROUTER_BBOX(i->data);
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
  printf("CLUSTER: %d]\n", box->cluster ? box->cluster->id : -1);
  
}

void
print_vertex(toporouter_vertex_t *v)
{
  if(v)
    printf("[V %f,%f,%f ", vx(v), vy(v), vz(v));
  else
    printf("[V (null) ");
  
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
      return ((PinType *)box->data)->Thickness + 1.;
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

  if(!box || !box->cluster) return Settings.LineThickness + 1.;
  
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
  
  if(!box || !box->cluster) return Settings.Keepaway + 1.;
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
  return (rval > 0.0001) ? 1 : ((rval < -0.0001) ? -1 : 0);
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
  return (rval > 0.0001) ? 1 : ((rval < -0.0001) ? -1 : 0);
}

inline int
vertex_wind(GtsVertex *a, GtsVertex *b, GtsVertex *c) 
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
fake_min_spacing(toporouter_vertex_t *v, toporouter_vertex_t *term, gdouble ms)
{
  /* term has a fakev */
  gdouble a,b,c,theta,beta,alpha,r,x,y;

#ifdef SPACING_DEBUG
  printf("Getting fake min spacing..\n");
#endif

  if(!vertex_wind(GTS_VERTEX(v), GTS_VERTEX(term), GTS_VERTEX(term->fakev)))
    return ms;

  vertex_move_towards_vertex_values(GTS_VERTEX(term), GTS_VERTEX(v), ms, &x, &y);
  
  a = gts_point_distance(GTS_POINT(term), GTS_POINT(term->fakev));
  c = sqrt(pow(x-vx(term),2) + pow(y-vy(term),2));//gts_point_distance(GTS_POINT(term), GTS_POINT(v));
  b = sqrt(pow(x-vx(term->fakev),2) + pow(y-vy(term->fakev),2));//gts_point_distance(GTS_POINT(term), GTS_POINT(v));
//  b = gts_point_distance(GTS_POINT(term->fakev), GTS_POINT(v));

  theta = acos((pow(b,2) - pow(a,2) - pow(c,2)) / (-2. * a * c));

  beta = asin(a * sin(theta) / ms);

  alpha = M_PI - theta - beta;

  r = sqrt(pow(ms,2) + pow(a,2) - (2. * ms * a * cos(alpha)));

#ifdef SPACING_DEBUG
  printf("fake_min_spacing: v = %f,%f term = %f,%f fakev = %f,%f ms = %f a = %f b = %f c = %f r = %f\n",
      vx(v), vy(v), vx(term), vy(term), vx(term->fakev), vy(term->fakev),
      ms, a, b, c, r);
#endif
  return r;
}

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
/*
  if(e) {
    if((v1 == tedge_v1(e) || v1 == tedge_v2(e)) && v1->fakev) {
//      if(v1->flags & VERTEX_FLAG_FAKEV_OUTSIDE_SEG)
        return fake_min_spacing(v2, v1, ms);
    }


    if((v2 == tedge_v1(e) || v2 == tedge_v2(e)) && v2->fakev) {
//      if(v2->flags & VERTEX_FLAG_FAKEV_OUTSIDE_SEG)
        return fake_min_spacing(v1, v2, ms);
    }
  }
*/
  return ms;
}

// v1 is a vertex in the CDT, and v2 is a net
inline gdouble
min_vertex_net_spacing(toporouter_vertex_t *v1, toporouter_vertex_t *v2)
{

  gdouble v1halfthick, v2halfthick, v1keepaway, v2keepaway, ms;
  toporouter_edge_t *e = v1->routingedge;
  
  v1halfthick = vertex_net_thickness(TOPOROUTER_VERTEX(v1)) / 2.;
  v2halfthick = cluster_thickness(vertex_bbox(v2)->cluster) / 2.;

  v1keepaway = vertex_net_keepaway(TOPOROUTER_VERTEX(v1));
  v2keepaway = cluster_keepaway(vertex_bbox(v2)->cluster);

  ms = v1halfthick + v2halfthick + MAX(v1keepaway, v2keepaway);

  if(e) {
    if((v1 == tedge_v1(e) || v1 == tedge_v2(e)) && v1->fakev) {
      return fake_min_spacing(v2, v1, ms);
    }

    if((v2 == tedge_v1(e) || v2 == tedge_v2(e)) && v2->fakev) {
      return fake_min_spacing(v1, v2, ms);
    }
  }

  return ms;
}

inline gdouble
min_net_net_spacing(toporouter_vertex_t *v1, toporouter_vertex_t *v2)
{

  gdouble v1halfthick, v2halfthick, v1keepaway, v2keepaway, ms;
  toporouter_edge_t *e = v1->routingedge;
  
  v1halfthick = cluster_thickness(vertex_bbox(v1)->cluster) / 2.;
  v2halfthick = cluster_thickness(vertex_bbox(v2)->cluster) / 2.;

  v1keepaway = cluster_keepaway(vertex_bbox(v1)->cluster);
  v2keepaway = cluster_keepaway(vertex_bbox(v2)->cluster);

  ms = v1halfthick + v2halfthick + MAX(v1keepaway, v2keepaway);

  if(e) {
    if((v1 == tedge_v1(e) || v1 == tedge_v2(e)) && v1->fakev) {
      return fake_min_spacing(v2, v1, ms);
    }

    if((v2 == tedge_v1(e) || v2 == tedge_v2(e)) && v2->fakev) {
      return fake_min_spacing(v1, v2, ms);
    }
  }

  return ms;
}

void
toporouter_draw_cluster(toporouter_t *r, drawing_context_t *dc, toporouter_cluster_t *cluster, gdouble red, gdouble green, gdouble blue, guint layer)
{
#if TOPO_OUTPUT_ENABLED
  GSList *i = cluster->i;

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

#if GLIB_MAJOR_VERSION <= 2 && GLIB_MINOR_VERSION < 14
typedef struct _GHashNode      GHashNode;

struct _GHashNode
{
  gpointer   key;
  gpointer   value;

  /* If key_hash == 0, node is not in use
   * If key_hash == 1, node is a tombstone
   * If key_hash >= 2, node contains data */
  guint      key_hash;
};

GList *
g_hash_table_get_keys (GHashTable *hash_table)
{
  gint i;
  GList *retval;

  g_return_val_if_fail (hash_table != NULL, NULL);

  retval = NULL;
  for (i = 0; i < hash_table->size; i++)
    {
      GHashNode *node = &hash_table->nodes [i];

      if (node->key_hash > 1)
        retval = g_list_prepend (retval, node->key);
    }

  return retval;
}
#endif


void
toporouter_draw_surface(toporouter_t *r, GtsSurface *s, char *filename, int w, int h, int mode, GSList *datas, int layer, GSList *candidatepoints) 
{
#if TOPO_OUTPUT_ENABLED
  drawing_context_t *dc;
  GSList *i; 
  toporouter_vertex_t *tv, *tv2 = NULL;

  dc = toporouter_output_init(w, h, filename);
  dc->mode = mode;
  dc->data = NULL;

  gts_surface_foreach_edge(s, toporouter_draw_edge, dc);
  gts_surface_foreach_vertex(s, toporouter_draw_vertex, dc);

  i = r->paths;
  while(i) {
    GSList *j = (GSList *) i->data;
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

    GSList *i;//, *k;

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
  g_slist_free(l->vertices);
  g_slist_free(l->constraints);

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
delaunay_create_from_vertices(GSList *vertices, GtsSurface **surface, GtsTriangle **t) 
{
//  GtsSurface *surface;
  GSList *i;
//  GtsTriangle *t;
  GtsVertex *v1, *v2, *v3;

  *t = gts_triangle_enclosing (gts_triangle_class (), vertices, 100000.0f);
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

//  return surface;
}

toporouter_bbox_t *
toporouter_bbox_create_from_points(int layer, GSList *vertices, toporouter_term_t type, gpointer data)
{
  toporouter_bbox_t *bbox;

//  delaunay_create_from_vertices(vertices, &s, &t);
  bbox = TOPOROUTER_BBOX( gts_bbox_points(GTS_BBOX_CLASS(toporouter_bbox_class()), vertices) );
  bbox->type = type;
  bbox->data = data;

  bbox->surface = NULL;
  bbox->enclosing = NULL;

  bbox->layer = layer;

  bbox->point = NULL;
  bbox->realpoint = NULL;

  return bbox;
}

toporouter_bbox_t *
toporouter_bbox_create(int layer, GSList *vertices, toporouter_term_t type, gpointer data)
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

GSList *
g_slist_insert_unique(GSList *list, void *data)
{
  GSList *i;

  i = list;
  while(i) {
    if(i->data == data) return list;
    i = i->next;
  }

  return g_slist_prepend(list, data);
}

struct delaunay_remove_similar_segment_data {
  GtsSegment *seg;
  GtsSurface *s;
};

gint
delaunay_remove_similar_segment_func(gpointer item, gpointer data) 
{
  struct delaunay_remove_similar_segment_data *d = data;
  GtsPoint *p = GTS_POINT(item);

  if(p->x == GTS_POINT(d->seg->v1)->x && p->y == GTS_POINT(d->seg->v1)->y) {
    gts_delaunay_remove_vertex(d->s, GTS_VERTEX(item));
  }else if(p->x == GTS_POINT(d->seg->v2)->x && p->y == GTS_POINT(d->seg->v2)->y) {
    gts_delaunay_remove_vertex(d->s, GTS_VERTEX(item));
  }

  return 0;
}

void
delaunay_remove_similar_segment(GtsSurface *s, GtsSegment *seg)
{
  struct delaunay_remove_similar_segment_data d;
  d.seg = seg;
  d.s = s;

  gts_surface_foreach_vertex(s, delaunay_remove_similar_segment_func, &d);

}

void 
toporouter_edge_remove(GtsEdge *edge) 
{
  edge->segment.v1->segments = g_slist_remove(edge->segment.v1->segments, &edge->segment);
  edge->segment.v2->segments = g_slist_remove(edge->segment.v2->segments, &edge->segment);
  //TODO: edge_destroy(edge); 
}

//#define TRACE_INS_CONST 1

GSList *
insert_constraint_edge_rec(toporouter_t *r, toporouter_layer_t *l, GtsVertex **v, toporouter_bbox_t *box)
{
  GtsEdgeClass *edge_class = GTS_EDGE_CLASS (toporouter_constraint_class ());
  GSList *i, *ii, *rlist = NULL, *tempconstraints;
  toporouter_constraint_t *tc;
  GtsEdge *e[2];
  GtsVertex *newv, *tempv;
  GtsVertex *p[2];
  gdouble wind[4];
  int j;

  g_assert(v[0] != v[1]);

#ifdef TRACE_INS_CONST
  fprintf(stderr, "entering insert_constraint_edge_rec v0 = %f,%f v1 = %f,%f\n",
      v[0]->p.x, v[0]->p.y,
      v[1]->p.x, v[1]->p.y);
#endif 

  /* detect edge intersections */
  i = l->constraints;
  while(i) {
    tc = i->data;

    for(j=0;j<2;j++) 
      wind[j] = vertex_wind(v[j], GTS_SEGMENT(tc)->v1, GTS_SEGMENT(tc)->v2);

    wind[2] = vertex_wind(v[0], v[1], GTS_SEGMENT(tc)->v1);
    wind[3] = vertex_wind(v[0], v[1], GTS_SEGMENT(tc)->v2);


    if(wind[0] == 0 && wind[1] == 0) {
      /* both points colinear */
      
#ifdef TRACE_INS_CONST
      fprintf(stderr, "\tboth points colinear with %f,%f  -  %f,%f\n",
          tc->c.edge.segment.v1->p.x, tc->c.edge.segment.v1->p.y,
          tc->c.edge.segment.v2->p.x, tc->c.edge.segment.v2->p.y);
#endif 
      
      if(v[0] == GTS_SEGMENT(tc)->v1 && v[1] != GTS_SEGMENT(tc)->v2 && 
        vertex_between(GTS_SEGMENT(tc)->v1, GTS_SEGMENT(tc)->v2, v[1]) ) {
#ifdef TRACE_INS_CONST
          fprintf(stderr, "\t\tv[0] == tc->v1 and v[1] in tc\n");
#endif
          l->constraints = g_slist_remove(l->constraints, tc);

          e[0] = gts_edge_new (edge_class, GTS_SEGMENT(tc)->v2, v[1]);
          TOPOROUTER_CONSTRAINT(e[0])->box = tc->box;
          l->constraints = g_slist_prepend (l->constraints, e[0]);
          
          gts_delaunay_add_vertex(tc->box->surface, v[1], NULL);

          tc->box->constraints = g_slist_remove(tc->box->constraints, tc);
          tc->box->constraints = g_slist_prepend(tc->box->constraints, e[0]);
          
          toporouter_edge_remove(GTS_EDGE(tc));
          
          tempconstraints = insert_constraint_edge_rec(r, l, v, box);
          ii = tempconstraints;
          while(ii) {
            if(GTS_VERTEX(GTS_SEGMENT(ii->data)->v1) == v[0] && GTS_VERTEX(GTS_SEGMENT(ii->data)->v2) == v[1]) {
              tc->box->constraints = g_slist_prepend(tc->box->constraints, TOPOROUTER_CONSTRAINT(ii->data));
              break;
            }else if(GTS_VERTEX(GTS_SEGMENT(ii->data)->v1) == v[1] && GTS_VERTEX(GTS_SEGMENT(ii->data)->v2) == v[0]) {
              tc->box->constraints = g_slist_prepend(tc->box->constraints, TOPOROUTER_CONSTRAINT(ii->data));
              break;
            }

            ii = ii->next;
          }

          return tempconstraints;

      }else if(v[0] == GTS_SEGMENT(tc)->v2 && v[1] != GTS_SEGMENT(tc)->v1 && 
        vertex_between(GTS_SEGMENT(tc)->v1, GTS_SEGMENT(tc)->v2, v[1]) ) {
#ifdef TRACE_INS_CONST
          fprintf(stderr, "\t\tv[0] == tc->v2 and v[1] in tc\n");
#endif 

          l->constraints = g_slist_remove(l->constraints, tc);

          e[0] = gts_edge_new (edge_class, GTS_SEGMENT(tc)->v1, v[1]);
          TOPOROUTER_CONSTRAINT(e[0])->box = tc->box;
          l->constraints = g_slist_prepend (l->constraints, e[0]);

          gts_delaunay_add_vertex(tc->box->surface, v[1], NULL);

          tc->box->constraints = g_slist_remove(tc->box->constraints, tc);
          tc->box->constraints = g_slist_prepend(tc->box->constraints, e[0]);
          
          toporouter_edge_remove(GTS_EDGE(tc));
          
          tempconstraints = insert_constraint_edge_rec(r, l, v, box);
          ii = tempconstraints;
          while(ii) {
            if(GTS_VERTEX(GTS_SEGMENT(ii->data)->v1) == v[0] && GTS_VERTEX(GTS_SEGMENT(ii->data)->v2) == v[1]) {
              tc->box->constraints = g_slist_prepend(tc->box->constraints, TOPOROUTER_CONSTRAINT(ii->data));
              break;
            }else if(GTS_VERTEX(GTS_SEGMENT(ii->data)->v1) == v[1] && GTS_VERTEX(GTS_SEGMENT(ii->data)->v2) == v[0]) {
              tc->box->constraints = g_slist_prepend(tc->box->constraints, TOPOROUTER_CONSTRAINT(ii->data));
              break;
            }

            ii = ii->next;
          }

          return tempconstraints;

      }else if(v[1] == GTS_SEGMENT(tc)->v1 && v[0] != GTS_SEGMENT(tc)->v2 && 
        vertex_between(GTS_SEGMENT(tc)->v1, GTS_SEGMENT(tc)->v2, v[0] )) {
#ifdef TRACE_INS_CONST
          fprintf(stderr, "\t\tv[1] == tc->v1 and v[0] in tc\n");
#endif 

          l->constraints = g_slist_remove(l->constraints, tc);

          e[0] = gts_edge_new (edge_class, GTS_SEGMENT(tc)->v2, v[0]);
          TOPOROUTER_CONSTRAINT(e[0])->box = tc->box;
          l->constraints = g_slist_prepend (l->constraints, e[0]);
          
          gts_delaunay_add_vertex(tc->box->surface, v[0], NULL);

          tc->box->constraints = g_slist_remove(tc->box->constraints, tc);
          tc->box->constraints = g_slist_prepend(tc->box->constraints, e[0]);
          
          toporouter_edge_remove(GTS_EDGE(tc));
          
          tempconstraints = insert_constraint_edge_rec(r, l, v, box);
          ii = tempconstraints;
          while(ii) {
            if(GTS_VERTEX(GTS_SEGMENT(ii->data)->v1) == v[0] && GTS_VERTEX(GTS_SEGMENT(ii->data)->v2) == v[1]) {
              tc->box->constraints = g_slist_prepend(tc->box->constraints, TOPOROUTER_CONSTRAINT(ii->data));
              break;
            }else if(GTS_VERTEX(GTS_SEGMENT(ii->data)->v1) == v[1] && GTS_VERTEX(GTS_SEGMENT(ii->data)->v2) == v[0]) {
              tc->box->constraints = g_slist_prepend(tc->box->constraints, TOPOROUTER_CONSTRAINT(ii->data));
              break;
            }

            ii = ii->next;
          }

          return tempconstraints;
      
      }else if(v[1] == GTS_SEGMENT(tc)->v2 && v[0] != GTS_SEGMENT(tc)->v1 && 
        vertex_between(GTS_SEGMENT(tc)->v1, GTS_SEGMENT(tc)->v2, v[0])) {
#ifdef TRACE_INS_CONST
          fprintf(stderr, "\t\tv[1] == tc->v2 and v[0] in tc\n");
#endif 

          l->constraints = g_slist_remove(l->constraints, tc);

          e[0] = gts_edge_new (edge_class, GTS_SEGMENT(tc)->v1, v[0]);
          TOPOROUTER_CONSTRAINT(e[0])->box = tc->box;
          l->constraints = g_slist_prepend (l->constraints, e[0]);
          
          gts_delaunay_add_vertex(tc->box->surface, v[0], NULL);

          tc->box->constraints = g_slist_remove(tc->box->constraints, tc);
          tc->box->constraints = g_slist_prepend(tc->box->constraints, e[0]);
          
          toporouter_edge_remove(GTS_EDGE(tc));
          
          tempconstraints = insert_constraint_edge_rec(r, l, v, box);
          ii = tempconstraints;
          while(ii) {
            if(GTS_VERTEX(GTS_SEGMENT(ii->data)->v1) == v[0] && GTS_VERTEX(GTS_SEGMENT(ii->data)->v2) == v[1]) {
              tc->box->constraints = g_slist_prepend(tc->box->constraints, TOPOROUTER_CONSTRAINT(ii->data));
              break;
            }else if(GTS_VERTEX(GTS_SEGMENT(ii->data)->v1) == v[1] && GTS_VERTEX(GTS_SEGMENT(ii->data)->v2) == v[0]) {
              tc->box->constraints = g_slist_prepend(tc->box->constraints, TOPOROUTER_CONSTRAINT(ii->data));
              break;
            }

            ii = ii->next;
          }

          return tempconstraints;

      }else if(v[0] != GTS_SEGMENT(tc)->v1 && v[1] != GTS_SEGMENT(tc)->v1 &&
          v[0] != GTS_SEGMENT(tc)->v2 && v[1] != GTS_SEGMENT(tc)->v2 &&
          vertex_between(GTS_SEGMENT(tc)->v1, GTS_SEGMENT(tc)->v2, v[0]) &&
          vertex_between(GTS_SEGMENT(tc)->v1, GTS_SEGMENT(tc)->v2, v[1]) 
          ){
#ifdef TRACE_INS_CONST
        fprintf(stderr, "\t\twithin segment\n");
#endif 
      
        l->constraints = g_slist_remove(l->constraints, tc);
        
        if(gts_point_distance(&GTS_SEGMENT(tc)->v1->p, &(v[0]->p)) < 
            gts_point_distance(&GTS_SEGMENT(tc)->v1->p, &(v[1]->p))) {
          p[0] = v[0];
          p[1] = v[1];
        }else{
          p[0] = v[1];
          p[1] = v[0];
        }

        e[0] = gts_edge_new (edge_class, GTS_SEGMENT(tc)->v1, p[0]);
        e[1] = gts_edge_new (edge_class, GTS_SEGMENT(tc)->v2, p[1]);
        TOPOROUTER_CONSTRAINT(e[0])->box = TOPOROUTER_CONSTRAINT(e[1])->box = tc->box;
        l->constraints = g_slist_prepend (l->constraints, e[0]);
        l->constraints = g_slist_prepend (l->constraints, e[1]);
        
        gts_delaunay_add_vertex(tc->box->surface, p[0], NULL);
        gts_delaunay_add_vertex(tc->box->surface, p[1], NULL);

        tc->box->constraints = g_slist_remove(tc->box->constraints, tc);
        tc->box->constraints = g_slist_prepend(tc->box->constraints, e[0]);
        tc->box->constraints = g_slist_prepend(tc->box->constraints, e[1]);
        
        toporouter_edge_remove(GTS_EDGE(tc));
        
        tempconstraints = insert_constraint_edge_rec(r, l, v, box);
        ii = tempconstraints;
        while(ii) {
          if(GTS_VERTEX(GTS_SEGMENT(ii->data)->v1) == p[0] && GTS_VERTEX(GTS_SEGMENT(ii->data)->v2) == p[1]) {
            tc->box->constraints = g_slist_prepend(tc->box->constraints, TOPOROUTER_CONSTRAINT(ii->data));
            break;
          }else if(GTS_VERTEX(GTS_SEGMENT(ii->data)->v1) == p[1] && GTS_VERTEX(GTS_SEGMENT(ii->data)->v2) == p[0]) {
            tc->box->constraints = g_slist_prepend(tc->box->constraints, TOPOROUTER_CONSTRAINT(ii->data));
            break;
          }

          ii = ii->next;
        }

        return tempconstraints;

      } 
    } 
    if( v[1] != GTS_SEGMENT(tc)->v2 && v[0] != GTS_SEGMENT(tc)->v2 &&
        v[1] != GTS_SEGMENT(tc)->v1 && v[0] != GTS_SEGMENT(tc)->v1 ) {
      
      if(!wind[0] && wind[1] && 
          vertex_between(GTS_SEGMENT(tc)->v1, GTS_SEGMENT(tc)->v2, v[0])) {

        /* v[0] lies on tc segment and v[1] does not */
#ifdef TRACE_INS_CONST
        fprintf(stderr, "\tv[0] lies on tc segment\n");
#endif 
        l->constraints = g_slist_remove(l->constraints, tc);

        e[0] = gts_edge_new (edge_class, GTS_SEGMENT(tc)->v1, v[0]);
        e[1] = gts_edge_new (edge_class, GTS_SEGMENT(tc)->v2, v[0]);
        TOPOROUTER_CONSTRAINT(e[0])->box = TOPOROUTER_CONSTRAINT(e[1])->box = tc->box;
        l->constraints = g_slist_prepend (l->constraints, e[0]);
        l->constraints = g_slist_prepend (l->constraints, e[1]);
        
        gts_delaunay_add_vertex(tc->box->surface, v[0], NULL);

        tc->box->constraints = g_slist_remove(tc->box->constraints, tc);
        tc->box->constraints = g_slist_prepend(tc->box->constraints, e[0]);
        tc->box->constraints = g_slist_prepend(tc->box->constraints, e[1]);
        
        toporouter_edge_remove(GTS_EDGE(tc));

        return insert_constraint_edge_rec(r, l, v, box);

      }else if(!wind[1] && wind[0] && 
          vertex_between(GTS_SEGMENT(tc)->v1, GTS_SEGMENT(tc)->v2, v[1])) {

        /* v[1] lies on tc segment and v[0] does not */
#ifdef TRACE_INS_CONST
        fprintf(stderr, "\tv[1] lies on tc segment\n");
#endif 
        l->constraints = g_slist_remove(l->constraints, tc);

        e[0] = gts_edge_new (edge_class, GTS_SEGMENT(tc)->v1, v[1]);
        e[1] = gts_edge_new (edge_class, GTS_SEGMENT(tc)->v2, v[1]);
        TOPOROUTER_CONSTRAINT(e[0])->box = TOPOROUTER_CONSTRAINT(e[1])->box = tc->box;
        l->constraints = g_slist_prepend (l->constraints, e[0]);
        l->constraints = g_slist_prepend (l->constraints, e[1]);
        
        gts_delaunay_add_vertex(tc->box->surface, v[1], NULL);

        tc->box->constraints = g_slist_remove(tc->box->constraints, tc);
        tc->box->constraints = g_slist_prepend(tc->box->constraints, e[0]);
        tc->box->constraints = g_slist_prepend(tc->box->constraints, e[1]);
        
        toporouter_edge_remove(GTS_EDGE(tc));

        return insert_constraint_edge_rec(r, l, v, box);

      }else if(!wind[2] && wind[3] && 
          vertex_between(v[0], v[1], GTS_SEGMENT(tc)->v1)) {

        /* tc v1 lies on segment, tc v2 does not */
#ifdef TRACE_INS_CONST
        fprintf(stderr, "\ttc v1 lies on segment, tc v2 does not\n");
        fprintf(stderr, "\ttc = %f,%f   -   %f,%f\n",
            GTS_SEGMENT(tc)->v1->p.x, GTS_SEGMENT(tc)->v1->p.y,
            GTS_SEGMENT(tc)->v2->p.x, GTS_SEGMENT(tc)->v2->p.y);
#endif 

        p[0] = GTS_SEGMENT(tc)->v1;
        p[1] = v[0];
        rlist = insert_constraint_edge_rec(r, l, p, box);
        p[1] = v[1];
        return g_slist_concat(rlist, insert_constraint_edge_rec(r, l, p, box));

      }else if(!wind[3] && wind[2] && 
          vertex_between(v[0], v[1], GTS_SEGMENT(tc)->v2)) {

        /* tc v2 lies on segment, tc v1 does not */
#ifdef TRACE_INS_CONST
        fprintf(stderr, "\ttc v2 lies on segment, tc v1 does not\n");
        fprintf(stderr, "\ttc = %f,%f   -   %f,%f\n",
            GTS_SEGMENT(tc)->v1->p.x, GTS_SEGMENT(tc)->v1->p.y,
            GTS_SEGMENT(tc)->v2->p.x, GTS_SEGMENT(tc)->v2->p.y);
#endif 

        p[0] = GTS_SEGMENT(tc)->v2;
        p[1] = v[0];
        rlist = insert_constraint_edge_rec(r, l, p, box);
        p[1] = v[1];
        return g_slist_concat(rlist, insert_constraint_edge_rec(r, l, p, box));

      }else if(vertex_intersect_prop(v[0], v[1], GTS_SEGMENT(tc)->v1, GTS_SEGMENT(tc)->v2)) {
        /* proper intersection */
#ifdef TRACE_INS_CONST
        fprintf(stderr, "\tproper intersection with  %f,%f  -  %f,%f\n",
            GTS_SEGMENT(tc)->v1->p.x, GTS_SEGMENT(tc)->v1->p.y,
            GTS_SEGMENT(tc)->v2->p.x, GTS_SEGMENT(tc)->v2->p.y);
#endif 

        l->constraints = g_slist_remove(l->constraints, tc);

        newv = vertex_intersect(v[0], v[1], GTS_SEGMENT(tc)->v1, GTS_SEGMENT(tc)->v2);

        ii = l->vertices;
        while (ii) {
          tempv = ii->data;
          if(tempv->p.x == newv->p.x && tempv->p.y == newv->p.y) {
            free(newv);
            newv = tempv;
            goto insert_constraint_proper_intersection_cont;
          }
          ii = ii->next;
        }
#ifdef TRACE_INS_CONST
        fprintf(stderr, "\tproceeding with new vertex\n");
#endif 
        TOPOROUTER_VERTEX(newv)->boxes = g_slist_insert_unique(NULL, box); 
        TOPOROUTER_VERTEX(newv)->boxes = g_slist_insert_unique(TOPOROUTER_VERTEX(newv)->boxes, tc->box); 
        l->vertices = g_slist_prepend(l->vertices, newv);

insert_constraint_proper_intersection_cont:

        e[0] = gts_edge_new (edge_class, GTS_SEGMENT(tc)->v1, newv);
        e[1] = gts_edge_new (edge_class, GTS_SEGMENT(tc)->v2, newv);
        TOPOROUTER_CONSTRAINT(e[0])->box = TOPOROUTER_CONSTRAINT(e[1])->box = tc->box;
        l->constraints = g_slist_prepend (l->constraints, e[0]);
        l->constraints = g_slist_prepend (l->constraints, e[1]);
        
        gts_delaunay_add_vertex(tc->box->surface, newv, NULL);

        tc->box->constraints = g_slist_remove(tc->box->constraints, tc);
        tc->box->constraints = g_slist_prepend(tc->box->constraints, e[0]);
        tc->box->constraints = g_slist_prepend(tc->box->constraints, e[1]);

        toporouter_edge_remove(GTS_EDGE(tc));

        p[0] = newv;
        p[1] = v[0];
        rlist = insert_constraint_edge_rec(r, l, p, box);
        p[1] = v[1];
        return g_slist_concat(rlist, insert_constraint_edge_rec(r, l, p, box));

      }
    }

    i = i->next;
  } 

  /* if we get to here, there were no problems, continue creating edge */
  
  /* check no points lie in edge */
  ii = l->vertices;
  while (ii) {
    tempv = GTS_VERTEX(ii->data);
    if(tempv != v[0] && tempv != v[1] && vertex_between(v[0], v[1], tempv)) {
#ifdef TRACE_INS_CONST
      fprintf(stderr, "\tpoint in edge\n");
#endif 
      p[0] = tempv;
      p[1] = v[0];
      rlist = insert_constraint_edge_rec(r, l, p, box);
      p[1] = v[1];
      return g_slist_concat(rlist, insert_constraint_edge_rec(r, l, p, box));
    }
    ii = ii->next;
  }

#ifdef TRACE_INS_CONST
  fprintf(stderr, "\tno probs, creating edge\n");
#endif 

  e[0] = gts_edge_new (edge_class, v[0], v[1]);
  TOPOROUTER_CONSTRAINT(e[0])->box = box;
  l->constraints = g_slist_prepend (l->constraints, e[0]);

  return g_slist_prepend(NULL, e[0]);
}

GtsVertex *
insert_vertex(toporouter_t *r, toporouter_layer_t *l, gdouble x, gdouble y, toporouter_bbox_t *box) 
{
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  GtsVertex *v;
  GSList *i;

  i = l->vertices;
  while (i) {
    v = i->data;
    if(v->p.x == x && v->p.y == y) 
      return v;
    i = i->next;
  }

  v = gts_vertex_new (vertex_class , x, y, l - r->layers);
  TOPOROUTER_VERTEX(v)->boxes = g_slist_prepend(NULL, box);
  l->vertices = g_slist_prepend(l->vertices, v);

  return v;
}

GSList *
insert_constraint_edge(toporouter_t *r, toporouter_layer_t *l, gdouble x1, gdouble y1, guint flags1, gdouble x2, gdouble y2, guint flags2, toporouter_bbox_t *box)
{
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  GtsEdgeClass *edge_class = GTS_EDGE_CLASS (toporouter_constraint_class ());
  GtsVertex *p[2];
  GtsVertex *v;
  GSList *i;
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
    TOPOROUTER_VERTEX(p[0])->boxes = g_slist_prepend(NULL, box);
    l->vertices = g_slist_prepend(l->vertices, p[0]);
  }
  if(p[1] == NULL) {
    p[1] = gts_vertex_new (vertex_class, x2, y2, l - r->layers);
    TOPOROUTER_VERTEX(p[1])->boxes = g_slist_prepend(NULL, box);
    l->vertices = g_slist_prepend(l->vertices, p[1]);
  }

  TOPOROUTER_VERTEX(p[0])->flags = flags1;
  TOPOROUTER_VERTEX(p[1])->flags = flags2;
  
  e = gts_edge_new (edge_class, p[0], p[1]);
  TOPOROUTER_CONSTRAINT(e)->box = box;
  l->constraints = g_slist_prepend (l->constraints, e);
  //return insert_constraint_edge_rec(r, l, p, box);
  return g_slist_prepend(NULL, e);

}

void
insert_constraints_from_list(toporouter_t *r, toporouter_layer_t *l, GSList *vlist, toporouter_bbox_t *box) 
{
  GSList *i = vlist;
  toporouter_vertex_t *pv = NULL, *v;

  while(i) {
    v = TOPOROUTER_VERTEX(i->data);

    if(pv) {
      box->constraints = 
        g_slist_concat(box->constraints, insert_constraint_edge(r, l, vx(v), vy(v), v->flags, vx(pv), vy(pv), pv->flags, box));
    }

    pv = v;
    i = i->next;
  }
  
  v = TOPOROUTER_VERTEX(vlist->data);
  box->constraints = 
    g_slist_concat(box->constraints, insert_constraint_edge(r, l, vx(v), vy(v), v->flags, vx(pv), vy(pv), pv->flags, box));

}

void
insert_centre_point(toporouter_t *r, toporouter_layer_t *l, gdouble x, gdouble y)
{
  GSList *i;
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());

  i = l->vertices;
  while (i) {
    GtsPoint *p = GTS_POINT(i->data);
    if(p->x == x && p->y == y) 
      return;
    i = i->next;
  }
  
  l->vertices = g_slist_prepend(l->vertices, gts_vertex_new(vertex_class, x, y, 0.0f));
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

GSList *
rect_with_attachments(gdouble rad,
    gdouble x0, gdouble y0,
    gdouble x1, gdouble y1,
    gdouble x2, gdouble y2,
    gdouble x3, gdouble y3,
    gdouble z)
{
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  GSList *r = NULL, *rr = NULL, *i;
  toporouter_vertex_t *curpoint, *temppoint;


  curpoint = TOPOROUTER_VERTEX(gts_vertex_new (vertex_class, x0, y0, z));

  r = g_slist_prepend(NULL, curpoint);
  r = g_slist_prepend(r, gts_vertex_new (vertex_class, x1, y1, z));
  r = g_slist_prepend(r, gts_vertex_new (vertex_class, x2, y2, z));
  r = g_slist_prepend(r, gts_vertex_new (vertex_class, x3, y3, z));
  
  i = r;
  while(i) {
    temppoint = TOPOROUTER_VERTEX(i->data);
    rr = g_slist_prepend(rr, curpoint);
    
    curpoint = temppoint;
    i = i->next;
  }

  g_slist_free(r);

  return rr;
}

#define VERTEX_CENTRE(x) TOPOROUTER_VERTEX( vertex_bbox(x)->point )

/*
 * Read pad data from layer into toporouter_layer_t struct
 *
 * Inserts points and constraints into GSLists
 */
int
read_pads(toporouter_t *r, toporouter_layer_t *l, guint layer) 
{
  toporouter_spoint_t p[2], rv[5];
  gdouble x[2], y[2], t, m;
  GSList *objectconstraints;

  GSList *vlist = NULL;
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

//            vlist = g_slist_prepend(NULL, gts_vertex_new (vertex_class, x[0]-t, y[0]-t, 0.));
//            vlist = g_slist_prepend(vlist, gts_vertex_new (vertex_class, x[0]-t, y[0]+t, 0.));
//            vlist = g_slist_prepend(vlist, gts_vertex_new (vertex_class, x[0]+t, y[0]+t, 0.));
//            vlist = g_slist_prepend(vlist, gts_vertex_new (vertex_class, x[0]+t, y[0]-t, 0.));
            vlist = rect_with_attachments(pad_rad(pad), 
                x[0]-t, y[0]-t, 
                x[0]-t, y[0]+t,
                x[0]+t, y[0]+t,
                x[0]+t, y[0]-t,
                l - r->layers);
            bbox = toporouter_bbox_create(l - r->layers, vlist, PAD, pad);
            r->bboxes = g_slist_prepend(r->bboxes, bbox);
            insert_constraints_from_list(r, l, vlist, bbox);
            g_slist_free(vlist);
            
            //bbox->point = GTS_POINT( gts_vertex_new(vertex_class, x[0], y[0], 0.) );
            bbox->point = GTS_POINT( insert_vertex(r, l, x[0], y[0], bbox) );

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
            
//            vlist = g_slist_prepend(NULL,  gts_vertex_new (vertex_class, rv[1].x, rv[1].y, 0.));
//            vlist = g_slist_prepend(vlist, gts_vertex_new (vertex_class, rv[2].x, rv[2].y, 0.));
//            vlist = g_slist_prepend(vlist, gts_vertex_new (vertex_class, rv[3].x, rv[3].y, 0.));
//            vlist = g_slist_prepend(vlist, gts_vertex_new (vertex_class, rv[4].x, rv[4].y, 0.));
            vlist = rect_with_attachments(pad_rad(pad), 
                rv[1].x, rv[1].y, 
                rv[2].x, rv[2].y,
                rv[3].x, rv[3].y,
                rv[4].x, rv[4].y,
                l - r->layers);
            bbox = toporouter_bbox_create(l - r->layers, vlist, PAD, pad);
            r->bboxes = g_slist_prepend(r->bboxes, bbox);
            insert_constraints_from_list(r, l, vlist, bbox);
            g_slist_free(vlist);
            
            //bbox->point = GTS_POINT( gts_vertex_new(vertex_class, (x[0] + x[1]) / 2., (y[0] + y[1]) / 2., 0.) );
            bbox->point = GTS_POINT( insert_vertex(r, l, (x[0] + x[1]) / 2., (y[0] + y[1]) / 2., bbox) );

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
            g_slist_free(vlist);
            
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
            g_slist_free(vlist);
            
            //bbox->point = GTS_POINT( gts_vertex_new(vertex_class, (x[0] + x[1]) / 2., (y[0] + y[1]) / 2., 0.) );
            bbox->point = GTS_POINT( insert_vertex(r, l, (x[0] + x[1]) / 2., (y[0] + y[1]) / 2., bbox) );
            
            //bbox->constraints = g_slist_concat(bbox->constraints, insert_constraint_edge(r, l, x[0], y[0], x[1], y[1], bbox));

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
 * Read points data (all layers) into GSList
 *
 * Inserts pin and via points
 */
int
read_points(toporouter_t *r, toporouter_layer_t *l, int layer)
{
  gdouble x, y, t;
  
  GSList *vlist = NULL;
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
        g_slist_free(vlist);
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
        g_slist_free(vlist);
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
      g_slist_free(vlist);
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
      g_slist_free(vlist);
        
      bbox->point = GTS_POINT( insert_vertex(r, l, x, y, bbox) );

    }
  }
  END_LOOP;
  return 0;
}

/*
 * Read line data from layer into toporouter_layer_t struct
 *
 * Inserts points and constraints into GSLists
 */
int
read_lines(toporouter_t *r, toporouter_layer_t *l, LayerType *layer, int ln) 
{
  gdouble xs[2], ys[2];
  
  GSList *vlist = NULL;
  toporouter_bbox_t *bbox = NULL;

  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  
  LINE_LOOP(layer);
  {
    xs[0] = line->Point1.X;
    xs[1] = line->Point2.X;
    ys[0] = line->Point1.Y;
    ys[1] = line->Point2.Y;
    if(!(xs[0] == xs[1] && ys[0] == ys[1])) {
      vlist = g_slist_prepend(NULL, gts_vertex_new (vertex_class, xs[0], ys[0], l - r->layers));
      vlist = g_slist_prepend(vlist, gts_vertex_new (vertex_class, xs[1], ys[1], l - r->layers));
      bbox = toporouter_bbox_create_from_points(GetLayerGroupNumberByNumber(ln), vlist, LINE, line);
      r->bboxes = g_slist_prepend(r->bboxes, bbox);
      g_slist_free(vlist);
      
      bbox->constraints = g_slist_concat(bbox->constraints, insert_constraint_edge(r, l, xs[0], ys[0], 0, xs[1], ys[1], 0, bbox));
    }
  }
  END_LOOP;
  
  return 0;
}



void
create_board_edge(gdouble x0, gdouble y0, gdouble x1, gdouble y1, gdouble max, gint layer, GSList **vlist)
{
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  gdouble d = sqrt(pow(x0-x1,2) + pow(y0-y1,2));
  guint n = d / max, count = 1;
  gdouble inc = n ? d / n : d;
  gdouble x = x0, y = y0;

  *vlist = g_slist_prepend(*vlist, gts_vertex_new (vertex_class, x0, y0, layer));
 
  while(count < n) {
    coord_move_towards_coord_values(x0, y0, x1, y1, inc, &x, &y);
    *vlist = g_slist_prepend(*vlist, gts_vertex_new (vertex_class, x, y, layer));

    x0 = x; y0 = y;
    count++;
  }

}


int
read_board_constraints(toporouter_t *r, toporouter_layer_t *l, int layer) 
{
//  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  GSList *vlist = NULL;
  toporouter_bbox_t *bbox = NULL;
  
  /* Add points for corners of board, and constrain those edges */
//  vlist = g_slist_prepend(NULL, gts_vertex_new (vertex_class, 0., 0., layer));
//  vlist = g_slist_prepend(vlist, gts_vertex_new (vertex_class, PCB->MaxWidth, 0., layer));
//  vlist = g_slist_prepend(vlist, gts_vertex_new (vertex_class, PCB->MaxWidth, PCB->MaxHeight, layer));
//  vlist = g_slist_prepend(vlist, gts_vertex_new (vertex_class, 0., PCB->MaxHeight, layer));

  create_board_edge(0., 0., PCB->MaxWidth, 0., 10000., layer, &vlist);
  create_board_edge(PCB->MaxWidth, 0., PCB->MaxWidth, PCB->MaxHeight, 10000., layer, &vlist);
  create_board_edge(PCB->MaxWidth, PCB->MaxHeight, 0., PCB->MaxHeight, 10000., layer, &vlist);
  create_board_edge(0., PCB->MaxHeight, 0., 0., 10000., layer, &vlist);
  
  bbox = toporouter_bbox_create(layer, vlist, BOARD, NULL);
  r->bboxes = g_slist_prepend(r->bboxes, bbox);
  insert_constraints_from_list(r, l, vlist, bbox);
  g_slist_free(vlist);

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
  GSList *i;
  GtsEdge *temp;
  GtsVertex *v;
  GtsTriangle *t;
  GtsVertex *v1, *v2, *v3;

  t = gts_triangle_enclosing (gts_triangle_class (), l->vertices, 1000.0f);
  gts_triangle_vertices (t, &v1, &v2, &v3);

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
  GSList *i;

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
  GSList *i = r->clusters;

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
    if(g_slist_find(cluster->i, box)) return;
    cluster->i = g_slist_prepend(cluster->i, box);
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
        r->clusters = g_slist_prepend(r->clusters, cluster);
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

      GROUP_LOOP(PCB->Data, group)
      {

#ifdef DEBUG_IMPORT    
        printf("reading pads from layer %d into group %d\n", number, group);
#endif
        read_pads(r, cur_layer, number);
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

//  printf("FINDING\n\n");

  while(hits) {
    toporouter_bbox_t *box = TOPOROUTER_BBOX(hits->data);
    
//    printf("HIT BOX: "); print_bbox(box);

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
  
//  printf("cluster_find: %f,%f,%f: ", x, y, z);
//  print_cluster(rval);

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

#define closelist_insert(p) closelist = g_slist_prepend(closelist, p)

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
print_path(GSList *path)
{

  printf("PATH:\n");
  while(path) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(path->data);
    printf("[V %f,%f,%f]\n", vx(v), vy(v), vz(v));

    path = path->next;
  }


}

GSList *
split_path(GSList *path) 
{
  toporouter_vertex_t *pv = NULL;
  GSList *curpath = NULL, *i, *paths = NULL;
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
      if(g_slist_length(curpath) > 1) paths = g_slist_prepend(paths, curpath);
      curpath = NULL;

      pv->child = NULL;
      v->parent = NULL;
    }
    
    curpath = g_slist_append(curpath, v);

    pv = v;
    i = i->next;
  }
  
  if(g_slist_length(curpath) > 1)
    paths = g_slist_prepend(paths, curpath);
  
  return paths;
}



#define edge_gradient(e) (cartesian_gradient(GTS_POINT(GTS_SEGMENT(e)->v1)->x, GTS_POINT(GTS_SEGMENT(e)->v1)->y, \
    GTS_POINT(GTS_SEGMENT(e)->v2)->x, GTS_POINT(GTS_SEGMENT(e)->v2)->y))


/* sorting into ascending distance from v1 */
gint  
routing_edge_insert(gconstpointer a, gconstpointer b, gpointer user_data)
{
  GtsPoint *v1 = GTS_POINT(edge_v1(user_data));

  if(gts_point_distance2(v1, GTS_POINT(a)) < gts_point_distance2(v1, GTS_POINT(b)))
    return -1;
  if(gts_point_distance2(v1, GTS_POINT(a)) > gts_point_distance2(v1, GTS_POINT(b)))
    return 1;

  printf("WARNING: routing_edge_insert() with same points..\n \
      v1 @ %f,%f\n\
      a  @ %f,%f\n\
      b  @ %f,%f\n", 
      v1->x, v1->y,
      vx(a), vy(a),
      vx(a), vy(b));

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
    if(vx(r) == x && vy(r) == y) {
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
    e->routing = g_slist_insert_sorted_with_data(e->routing, r, routing_edge_insert, e);
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

GSList *
cluster_vertices(toporouter_cluster_t *cluster)
{
  GSList *i = NULL, *rval = NULL;

  if(!cluster) return NULL;

  i = cluster->i;
  while(i) {
    toporouter_bbox_t *box = TOPOROUTER_BBOX(i->data);

    if(box->type == LINE) {
      g_assert(box->constraints->data);
      rval = g_slist_prepend(rval, tedge_v1(box->constraints->data));
      rval = g_slist_prepend(rval, tedge_v2(box->constraints->data));
    }else if(box->point) {
      rval = g_slist_prepend(rval, TOPOROUTER_VERTEX(box->point));
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
  GSList *src_vertices = cluster_vertices(routedata->src), *i = src_vertices;
  GSList *dest_vertices = cluster_vertices(routedata->dest), *j = dest_vertices;

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

  g_slist_free(src_vertices);
  g_slist_free(dest_vertices);
}


void
closest_cluster_pair(toporouter_t *r, GSList *src_vertices, GSList *dest_vertices, toporouter_vertex_t **a, toporouter_vertex_t **b)
{
  GSList *i = src_vertices, *j = dest_vertices;

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

//  g_slist_free(src_vertices);
//  g_slist_free(dest_vertices);
}


toporouter_vertex_t *
closest_dest_vertex(toporouter_t *r, toporouter_vertex_t *v, toporouter_route_t *routedata)
{
  GSList *vertices = cluster_vertices(routedata->dest), *i = vertices;
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

  g_slist_free(vertices);

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


GSList *
candidate_vertices(toporouter_vertex_t *v1, toporouter_vertex_t *v2, toporouter_vertex_t *dest, toporouter_edge_t *e) 
{
  gdouble totald, v1ms, v2ms, flow, capacity, ms;
  GSList *vs = NULL;

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
    vs = g_slist_prepend(vs, new_temp_toporoutervertex((vx(v1)+vx(v2)) / 2., (vy(v1)+vy(v2)) / 2., e));
  }else{
    gdouble x0, y0, x1, y1;
    
    vertex_move_towards_vertex_values(GTS_VERTEX(v1), GTS_VERTEX(v2), v1ms, &x0, &y0);
    
    vs = g_slist_prepend(vs, new_temp_toporoutervertex(x0, y0, e));
    
    vertex_move_towards_vertex_values(GTS_VERTEX(v2), GTS_VERTEX(v1), v2ms, &x1, &y1);
    
    vs = g_slist_prepend(vs, new_temp_toporoutervertex(x1, y1, e));
    
    if(ms < sqrt(pow(x0-x1,2) + pow(y0-y1,2))) 
      vs = g_slist_prepend(vs, new_temp_toporoutervertex((x0+x1) / 2., (y0+y1) / 2., e));

  }
#ifdef DEBUG_ROUTE
  printf("candidate vertices returning %d\n", g_slist_length(vs));
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

GSList *
triangle_candidate_points_from_vertex(GtsTriangle *t, toporouter_vertex_t *v, toporouter_vertex_t *dest)
{
  toporouter_edge_t *op_e = TOPOROUTER_EDGE(gts_triangle_edge_opposite(t, GTS_VERTEX(v)));
  toporouter_vertex_t *vv1, *vv2, *constraintv = NULL;
  toporouter_edge_t *e1, *e2;
  GList *i;
  GSList *rval = NULL;

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
//    return g_slist_prepend(NULL, vv1);

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
    return g_slist_prepend(NULL, constraintv);  
  }

  i = edge_routing(op_e);
  while(i) {
    toporouter_vertex_t *temp = TOPOROUTER_VERTEX(i->data);

    if(temp->parent == v || temp->child == v) {
      rval = g_slist_concat(rval, candidate_vertices(vv1, temp, dest, op_e)); 
      vv1 = temp;
    }

    i = i->next;
  }

  rval = g_slist_concat(rval, candidate_vertices(vv1, vv2, dest, op_e)); 

  return rval; 



triangle_candidate_points_from_vertex_exit:
  if(constraintv) delete_vertex(constraintv);    

  g_slist_free(rval);

  return NULL;
}




GSList *
triangle_candidate_points_from_edge(toporouter_t *r, GtsTriangle *t, toporouter_edge_t *e, toporouter_vertex_t *v, toporouter_vertex_t **dest,
    toporouter_route_t *routedata)
{
  toporouter_vertex_t *v1, *v2, *op_v, *vv = NULL, *e1constraintv = NULL, *e2constraintv = NULL;
  toporouter_edge_t *e1, *e2;
  GSList *e1cands = NULL, *e2cands = NULL, *rval = NULL;
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
      printf("v1 putting in constraint.. dest netlist = %s, constraint netlist = %s\n", 
          TOPOROUTER_CONSTRAINT(e1)->box->netlist,
          vertex_bbox(*dest)->netlist);
#endif
      tempv = new_temp_toporoutervertex_in_segment(e1, tedge_v1(e1), gts_point_distance(GTS_POINT(edge_v1(e1)), GTS_POINT(edge_v2(e1))) / 2., tedge_v2(e1));
//      e1cands = g_slist_prepend(NULL, tempv);
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
        rval = g_slist_prepend(rval, op_v);
      }
    }else{
      if(g_slist_find(routedata->destvertices, op_v)) {
        rval = g_slist_prepend(rval, op_v);
      }else if(g_slist_find(routedata->destvertices, boxpoint)) {
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
      //e2cands = g_slist_prepend(NULL, tempv);
      
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
    g_slist_free(e1cands);
    e1cands = NULL;
  }
  
  if(noe2 || !check_triangle_interior_capacity(t, v2, v, e1, e, e2)) {
#ifdef DEBUG_ROUTE      
    printf("freeing e2cands\n");
#endif
    g_slist_free(e2cands);
    e2cands = NULL;
  }

  if(!noe1 && e1constraintv) {
    e1cands = g_slist_prepend(e1cands, e1constraintv);
  }else if(e1constraintv) {
    delete_vertex(e1constraintv);
  }

  if(!noe2 && e2constraintv) {
    e2cands = g_slist_prepend(e2cands, e2constraintv);
  }else if(e2constraintv) {
    delete_vertex(e2constraintv);
  }
  
  if(!noe1 && !noe2) return g_slist_concat(rval, g_slist_concat(e1cands, e2cands));

  return g_slist_concat(e1cands, e2cands);
}

GSList *
compute_candidate_points(toporouter_t *tr, toporouter_layer_t *l, toporouter_vertex_t *curpoint, toporouter_route_t *data,
    toporouter_vertex_t **closestdest) 
{
  GSList *r = NULL, *i, *j;
  GSList *triangles;
  toporouter_edge_t *edge = curpoint->routingedge, *tempedge;
  
  if(!(curpoint->flags & VERTEX_FLAG_TEMP)) {

    GSList *vertices = gts_vertex_neighbors(GTS_VERTEX(curpoint), NULL, NULL);

    i = vertices;
    while(i) {
      toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);

      if(TOPOROUTER_IS_CONSTRAINT(gts_vertices_are_connected(GTS_VERTEX(curpoint), GTS_VERTEX(v)))) r = g_slist_prepend(r, v);        

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

  /* direct connection */
//  if(curpoint == TOPOROUTER_VERTEX(data->src->point))
  if((tempedge = TOPOROUTER_EDGE(gts_vertices_are_connected(GTS_VERTEX(curpoint), GTS_VERTEX(*closestdest))))) {
    //printf("ATTEMPTING DIRECT CONNECT\n");
    
    if(TOPOROUTER_IS_CONSTRAINT(tempedge)) {
      goto compute_candidate_points_finish;
    }else{
      if(!tempedge->routing) {
        r = g_slist_prepend(NULL, *closestdest);
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
    i = triangles = gts_vertex_triangles(GTS_VERTEX(curpoint), NULL);
#ifdef DEBUG_ROUTE    
    printf("triangle count = %d\n", g_slist_length(triangles));
#endif    
    while(i) {
      GtsTriangle *t = GTS_TRIANGLE(i->data);

      //GtsEdge* e = gts_triangle_edge_opposite(GTS_TRIANGLE(i->data), GTS_VERTEX(curpoint));
      GSList *temppoints = triangle_candidate_points_from_vertex(t, curpoint, *closestdest);
#ifdef DEBUG_ROUTE     
      printf("\treturned %d points\n", g_slist_length(temppoints));
#endif      
      j = temppoints;
      while(j) {
        g_hash_table_insert(data->alltemppoints, j->data, NULL);  
        j = j->next;
      }

      r = g_slist_concat(r, temppoints);
      //triangle_check_visibility(&r, GTS_TRIANGLE(i->data), curpoint);
      i = i->next;
    }
    g_slist_free(triangles);
  }else /* a temp point */ {
    int prevwind = vertex_wind(GTS_SEGMENT(edge)->v1, GTS_SEGMENT(edge)->v2, GTS_VERTEX(curpoint->parent));
//    printf("tempoint\n");
    
    i = GTS_EDGE(edge)->triangles;
    while(i) {
      GtsVertex *oppv =  gts_triangle_vertex_opposite(GTS_TRIANGLE(i->data), GTS_EDGE(edge));
      if(prevwind != vertex_wind(GTS_SEGMENT(edge)->v1, GTS_SEGMENT(edge)->v2, oppv)) {
        GSList *temppoints;
/*
        if(oppv == GTS_VERTEX(closestdest)) {
          r = g_slist_prepend(r, closestdest);
        }else{

          // check zlinks of oppv 
          j = TOPOROUTER_VERTEX(oppv)->zlink;
          while(j) {
            if(TOPOROUTER_VERTEX(j->data) == TOPOROUTER_VERTEX(closestdest)) { 
              r = g_slist_prepend(r, oppv);
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
            g_hash_table_insert(data->alltemppoints, j->data, NULL); 
#ifdef DEBUG_ROUTE          
          else 
            printf("got cand not a temp\n");
#endif
          j = j->next;
        }
        r = g_slist_concat(r, temppoints);
        
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
        r = g_slist_prepend(r, i->data);
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
        GSList *i = data->srcvertices;
        while(i) {
          toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);
          if(v != curpoint && vx(v) == vx(curpoint) && vy(v) == vy(curpoint))
            r = g_slist_prepend(r, v);
          i = i->next;
        }
      }else{
        r = g_slist_concat(r, g_slist_copy(data->srcvertices));
      }
    }
  }

  return r;
}


gint       
clean_edge(gpointer item, gpointer data)
{
  toporouter_edge_t *e = TOPOROUTER_EDGE(item);
  toporouter_vertex_t *tv;
  GList *i = edge_routing(e);
  
  while(i) {
    tv = TOPOROUTER_VERTEX(i->data); 
    if(tv->flags & VERTEX_FLAG_TEMP) {
      if(TOPOROUTER_IS_CONSTRAINT(tv->routingedge)) 
        TOPOROUTER_CONSTRAINT(tv->routingedge)->routing = g_list_remove(TOPOROUTER_CONSTRAINT(tv->routingedge)->routing, tv);
      else
        tv->routingedge->routing = g_list_remove(tv->routingedge->routing, tv);
      gts_object_destroy ( GTS_OBJECT(tv) );
    }

    i = i->next;
  }

  return 0;  
}

void
clean_routing_edges(toporouter_t *r, toporouter_route_t *data)
{
  GList *j, *i;
  j = i = g_hash_table_get_keys(data->alltemppoints);
  while(i) {
    toporouter_vertex_t *tv = TOPOROUTER_VERTEX(i->data);
    if(tv->flags & VERTEX_FLAG_TEMP) {
      if(TOPOROUTER_IS_CONSTRAINT(tv->routingedge)) 
        TOPOROUTER_CONSTRAINT(tv->routingedge)->routing = g_list_remove(TOPOROUTER_CONSTRAINT(tv->routingedge)->routing, tv);
      else
        tv->routingedge->routing = g_list_remove(tv->routingedge->routing, tv);
      gts_object_destroy ( GTS_OBJECT(tv) );
    }
    i = i->next;
  }
  g_hash_table_destroy(data->alltemppoints);  
  g_list_free(j);
  data->alltemppoints = NULL;
  
  for(gint i=0;i<groupcount();i++) {
    gts_surface_foreach_edge(r->layers[i].surface, clean_edge, NULL);
  }
}

gdouble
path_score(toporouter_t *r, GSList *path)
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

#define vlayer(x) (&r->layers[(int)vz(x)])

toporouter_vertex_t *
route(toporouter_t *r, toporouter_route_t *data, guint debug)
{
  GtsEHeap *openlist = gts_eheap_new(route_heap_cmp, NULL);
  GSList *closelist = NULL;
  GSList *i;
  gint count = 0;
  toporouter_vertex_t *rval = NULL;

  toporouter_vertex_t *srcv = NULL, *destv = NULL, *curpoint = NULL;
  toporouter_layer_t *cur_layer, *dest_layer;

  g_assert(data->src != data->dest);
  
  data->destvertices = cluster_vertices(data->dest);
  data->srcvertices = cluster_vertices(data->src);

  closest_cluster_pair(r, data->srcvertices, data->destvertices, &curpoint, &destv);
  
  if(!curpoint || !destv) return NULL;

  srcv = curpoint;
  cur_layer = vlayer(curpoint); dest_layer = vlayer(destv);

#ifdef DEBUG_ROUTE

  printf("ROUTING NETLIST %s starting at %f,%f\n", vertex_bbox(curpoint)->netlist, 
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

  while(gts_eheap_size(openlist) > 0) {
    GSList *candidatepoints;
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
    
    if(g_slist_find(data->destvertices, curpoint)) {
      toporouter_vertex_t *temppoint = curpoint;

      if(data->path) {
        g_slist_free(data->path);
        data->path = NULL;
      }

      while(temppoint) {
        data->path = g_slist_prepend(data->path, temppoint);    
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
        GSList *datas = g_slist_prepend(NULL, data);
        sprintf(buffer, "route-%d-%05d.png", j, count);
        toporouter_draw_surface(r, r->layers[j].surface, buffer, 1024, 1024, 2, datas, j, candidatepoints);
        g_slist_free(datas);
      }
    }
        *********************/
#endif    
    count++;
//    if(count > 100) exit(0);
    i = candidatepoints;
    while(i) {
      toporouter_vertex_t *temppoint = TOPOROUTER_VERTEX(i->data);
      if(!g_slist_find(closelist, temppoint) && temppoint != curpoint) {

        gdouble temp_g_cost = curpoint->gcost 
          + gts_point_distance(GTS_POINT(curpoint), GTS_POINT(temppoint));

        toporouter_heap_search_data_t heap_search_data = { temppoint, NULL };

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
    g_slist_free(candidatepoints);

  }
#ifdef DEBUG_ROUTE  
  printf("ROUTE: could not find path!\n");
#endif 

  data->score = INFINITY;
  clean_routing_edges(r, data); 

  if(data->path) {
    g_slist_free(data->path);
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

          if(tedge_v2(tv->routingedge) != srcv && g_slist_find(data->srcvertices, tedge_v2(tv->routingedge))) 
            restartv = tedge_v2(tv->routingedge);
          else if(boxpoint != srcv && g_slist_find(data->srcvertices, boxpoint)) 
            restartv = boxpoint;
        }
        
        if(!list->prev) {
          if(vertex_bbox(tedge_v1(tv->routingedge))) 
            boxpoint = TOPOROUTER_VERTEX(vertex_bbox(tedge_v1(tv->routingedge))->point);
          else
            boxpoint = NULL;

          if(tedge_v1(tv->routingedge) != srcv && g_slist_find(data->srcvertices, tedge_v1(tv->routingedge))) 
            restartv = tedge_v1(tv->routingedge);
          else if(boxpoint != srcv && g_slist_find(data->srcvertices, boxpoint)) 
            restartv = boxpoint;
          
        }

        if(restartv) {
          clean_routing_edges(r, data); 
          gts_eheap_destroy(openlist);     
          g_slist_free(closelist);
          openlist = gts_eheap_new(route_heap_cmp, NULL);
          closelist = NULL;
          g_slist_free(data->path);
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
    i = data->path;
    while(i) {
      toporouter_vertex_t *tv = TOPOROUTER_VERTEX(i->data);
      
      if(pv && g_slist_find(data->srcvertices, tv)) {
        GSList *temp = g_slist_copy(i);
        g_slist_free(data->path);
        data->path = temp;
        i = data->path;
      }
      pv = tv;
      i = i->next;
    }
  }
  
  {
    toporouter_vertex_t *pv = NULL;
    i = data->path;
    while(i) {
      toporouter_vertex_t *tv = TOPOROUTER_VERTEX(i->data);
      if(tv->flags & VERTEX_FLAG_TEMP) { 
        tv->flags ^= VERTEX_FLAG_TEMP;
        tv->flags |= VERTEX_FLAG_ROUTE;
      }
      if(pv) pv->child = tv;
      pv = tv;
      i = i->next;
    }
  }
 
  {
    toporouter_vertex_t *pv = NULL, *v = NULL;

    i = data->path;
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

  g_slist_free(data->destvertices);
  g_slist_free(data->srcvertices);
  gts_eheap_destroy(openlist);     
  g_slist_free(closelist);

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


/* pushes vertex v towards vertex p on edge e 
 * ptv is previous vertex, ntv is next vertex on routing edge */
void
push_point(toporouter_vertex_t *v, GtsVertex *p, toporouter_edge_t *e) 
{
  /* determine direction */
//  int wind1 = vertex_wind(GTS_VERTEX(ptv), GTS_VERTEX(v), edge_v1(e));
//  int wind2 = vertex_wind(GTS_VERTEX(ptv), GTS_VERTEX(v), p);
  gdouble vp_d, force, sparespace, vnextv_d, minspace;
  toporouter_vertex_t *nextv;
  guint direction;
//  if(wind2 == 0) return;
//  g_assert(wind1 != 0);

  vp_d = gts_point_distance(GTS_POINT(v), GTS_POINT(p));
  force = vp_d * 0.1f;

  if(v == TOPOROUTER_VERTEX(edge_v1(e)) || v == TOPOROUTER_VERTEX(edge_v2(e))) return;

  g_assert(gts_point_distance(GTS_POINT(edge_v2(e)), GTS_POINT(v)) != gts_point_distance(GTS_POINT(edge_v2(e)), GTS_POINT(p))); 
  
  if(gts_point_distance(GTS_POINT(edge_v2(e)), GTS_POINT(v)) < gts_point_distance(GTS_POINT(edge_v2(e)), GTS_POINT(p))) 
    direction = 1;
  else 
    direction = 0;

  if(direction) {
    /* towards v1 */  
    GList *prev = g_list_find(edge_routing(e), v)->prev;
    nextv = prev ? TOPOROUTER_VERTEX(prev->data) : TOPOROUTER_VERTEX(edge_v1(e));  
  }else{
    /* towards v2 */
    GList *next = g_list_find(edge_routing(e), v)->next;
    nextv = next ? TOPOROUTER_VERTEX(next->data) : TOPOROUTER_VERTEX(edge_v2(e));  
  }
    
  vnextv_d = gts_point_distance(GTS_POINT(v), GTS_POINT(nextv));
  minspace = min_spacing(nextv, v);
  sparespace = vnextv_d - minspace;

//  if(vx(nextv) == 35001. && vy(nextv) == 45000.) {
//    printf("sparespace = %f\n", sparespace);

//  }

//  if(sparespace < 0. && sparespace > -EPSILON) sparespace = 0.;

  if(sparespace < 0.) {
    /* push backwards */
   
    force = fabs(sparespace) * 0.1;
    
    if(!direction) {
      /* towards v1 */  
      GList *prev = g_list_find(edge_routing(e), v)->prev;
      while(prev) {
        nextv = TOPOROUTER_VERTEX(prev->data);  
        vertex_move_towards_vertex(GTS_VERTEX(nextv), GTS_VERTEX(edge_v1(e)), force);
        prev = prev->prev;
      }
      vertex_move_towards_vertex(GTS_VERTEX(v), GTS_VERTEX(edge_v1(e)), force);
    }else{
      /* towards v2 */
      GList *next = g_list_find(edge_routing(e), v)->next;
      while(next) {
        nextv = TOPOROUTER_VERTEX(next->data);  
        vertex_move_towards_vertex(GTS_VERTEX(nextv), GTS_VERTEX(edge_v2(e)), force);
        next = next->next;
      }
      vertex_move_towards_vertex(GTS_VERTEX(v), GTS_VERTEX(edge_v2(e)), force);
    }

//  }else if(sparespace > -EPSILON && sparespace < EPSILON) {

//  }else if(force > sparespace) {
//    vertex_move_towards_vertex(GTS_VERTEX(v), GTS_VERTEX(p), 0.9 * sparespace);
//    vertex_move_towards_vertex(GTS_VERTEX(v), GTS_VERTEX(p), force);
//    if(vnextv_d < vp_d) {
//      v = nextv;
//      goto push_point_recurse;
//    }
  }else{
    vertex_move_towards_vertex(GTS_VERTEX(v), GTS_VERTEX(p), force);
  }

}

// projection of a onto b
inline void
vprojection(gdouble ax, gdouble ay, gdouble bx, gdouble by, gdouble *x, gdouble *y)
{
  gdouble m = ((ax * bx) + (ay * by)) / (pow(bx,2) + pow(by,2));
  *x = bx * m;
  *y = by * m;
}

gdouble 
vertices_min_spacing(toporouter_vertex_t *a, toporouter_vertex_t *b)
{
  GList *list = NULL;
  toporouter_edge_t *e;
  gdouble space = 0.;
  toporouter_vertex_t *v1, *v2, *n;

  if(!a->routingedge && !b->routingedge) return min_spacing(a, b);
  else if(b->routingedge && a->routingedge) {
    if(b->routingedge != a->routingedge) return NAN;
    
    e = a->routingedge;
    list = edge_routing(a->routingedge);
    if(g_list_index(list, a) < g_list_index(list, b)) {
      v1 = a; v2 = b;
      n = TOPOROUTER_VERTEX(g_list_first(list)->data);  
    }else{
      v1 = b; v2 = a;
      n = TOPOROUTER_VERTEX(g_list_find(list, v1)->data);  
    }
  }else if(a->routingedge) {
    e = a->routingedge;
    list = edge_routing(a->routingedge);
    if(b == tedge_v1(a->routingedge)) {
      v1 = b; v2 = a;
      n = TOPOROUTER_VERTEX(g_list_first(list)->data);  
    }else{
      v1 = a; v2 = b;
      n = TOPOROUTER_VERTEX(g_list_find(list, v1)->data);  
    }
  }else{
    e = b->routingedge;
    list = edge_routing(b->routingedge);
    if(a == tedge_v1(b->routingedge)) {
      v1 = a; v2 = b;
      n = TOPOROUTER_VERTEX(g_list_first(list)->data);  
    }else{
      v1 = b; v2 = a;
      n = TOPOROUTER_VERTEX(g_list_find(list, v1)->data);  
    }
  }

  g_assert(list);
  g_assert(e);


#ifdef DEBUG_EXPORT
  printf("v1 = %f,%f n = %f,%f v2 = %f,%f\n", vx(v1), vy(v1), vx(n), vy(n), vx(v2), vy(v2));
  printf("SPACE: ");
#endif

  if(v1 != n) {
    space += min_spacing(v1, n);
#ifdef DEBUG_EXPORT
    printf("%f ", space); 
#endif
  }

  while(n != v2) {
    toporouter_vertex_t *next = edge_routing_next(e, list);
    
    space += min_spacing(n, next);
#ifdef DEBUG_EXPORT
    printf("%f ", space); 
#endif

    n = next;
    list = list->next;
  }
#ifdef DEBUG_EXPORT
  printf("\n");
#endif
  return space;
}

gdouble 
coord_projection(gdouble ax, gdouble ay, gdouble bx, gdouble by)
{
  gdouble alen2, c;
  gdouble b1x, b1y;

  alen2 = pow(ax,2) + pow(ay,2);

  c = ((bx*ax)+(by*ay)) / alen2;

  b1x = c * ax;
  b1y = c * ay;

  return sqrt(pow(b1x,2) + pow(b1y,2));
}

// projection of b on a 
gdouble 
vertex_projection(toporouter_vertex_t *a, toporouter_vertex_t *b, toporouter_vertex_t *o)
{
  gdouble nax, nay, ax, ay, alen2, c;
  gdouble b1x, b1y;

  nax = vx(a) - vx(o);
  nay = vy(a) - vy(o);
  alen2 = pow(nax,2) + pow(nay,2);

  ax = vx(b) - vx(o);
  ay = vy(b) - vy(o);

  c = ((ax*nax)+(ay*nay)) / alen2;

  b1x = c * nax;
  b1y = c * nay;

  return sqrt(pow(b1x-vx(o),2) + pow(b1y-vy(o),2));
}

gdouble 
constraint_arc_min_spacing_projection(toporouter_edge_t *e, toporouter_vertex_t *v, toporouter_vertex_t *commonv, toporouter_vertex_t *cv,
    gdouble ms)
{
  toporouter_vertex_t *internalv, *edge_op_v;
  gdouble x1, y1, r, costheta, a2, b2, c2, ax, ay, bx, by; 

  if(cv->parent->routingedge == e) internalv = cv->child;
  else internalv = cv->parent;

//  ms = vertices_min_spacing(commonv, v); 

  vertex_move_towards_vertex_values(GTS_VERTEX(internalv), GTS_VERTEX(cv), 
      ms + gts_point_distance(GTS_POINT(cv), GTS_POINT(internalv)), &x1, &y1);

//  x1 = vx(commonv) + x1 - vx(cv);
//  y1 = vy(commonv) + y1 - vy(cv);

  if(tedge_v1(e) == commonv) edge_op_v = tedge_v2(e);
  else edge_op_v = tedge_v1(e);

  ax = x1 - vx(cv);
  ay = y1 - vy(cv);
  bx = vx(edge_op_v) - vx(commonv);
  by = vy(edge_op_v) - vy(commonv);


  a2 = pow(ax,2) + pow(ay,2);
  b2 = pow(bx,2) + pow(by,2);
  c2 = pow(ax-bx,2) + pow(ay-by,2);

  costheta = (c2 - a2 - b2) / (-2 * sqrt(a2) * sqrt(b2));

  r = sqrt(a2) / costheta;

//  vprojection(x1 - vx(cv), y1 - vy(cv), vx(edge_op_v) - vx(commonv), vy(edge_op_v) - vy(commonv), &px, &py);
//  r = coord_projection(vx(edge_op_v) - vx(commonv), vy(edge_op_v) - vy(commonv), x1 - vx(cv), y1 - vy(cv));
  printf("space = %f proj = %f a = %f b = %f\n", ms, r, sqrt(a2), sqrt(b2));
//  return sqrt(pow(px,2) + pow(py,2));
  return r;
}

gdouble
constraint_arc_min_spacing(toporouter_edge_t *e, toporouter_vertex_t *v, toporouter_vertex_t **edgev, gdouble ms)
{
  toporouter_vertex_t *first = TOPOROUTER_VERTEX(g_list_first(edge_routing(e))->data);
  toporouter_vertex_t *last = TOPOROUTER_VERTEX(g_list_last(edge_routing(e))->data);

  *edgev = tedge_v1(e);

  if(v != first && first->parent && TOPOROUTER_IS_CONSTRAINT(first->parent->routingedge)) { 
    printf("CON SPACING FIRST PARENT:\n");
    return constraint_arc_min_spacing_projection(e, v, tedge_v1(e), first->parent, ms);
  }
  if(v != first && first->child && TOPOROUTER_IS_CONSTRAINT(first->child->routingedge)) {
    printf("CON SPACING LAST PARENT:\n");
    return constraint_arc_min_spacing_projection(e, v, tedge_v1(e), first->child, ms);
  }
  if(last != first && last != v) {

    *edgev = tedge_v2(e);

    if(last->parent && TOPOROUTER_IS_CONSTRAINT(last->parent->routingedge)) {
      printf("CON SPACING LAST PARENT:\n");
      return constraint_arc_min_spacing_projection(e, v, tedge_v2(e), last->parent, ms);
    }
    if(last->child && TOPOROUTER_IS_CONSTRAINT(last->child->routingedge)) {
      printf("CON SPACING LAST CHILD:\n");
      return constraint_arc_min_spacing_projection(e, v, tedge_v2(e), last->child, ms);
    }
  }

  return NAN;
}

/* snaps vertex v to p */
void
vertex_snap(GtsVertex *v, GtsVertex *p) 
{

  if(vx(v) > vx(p) - EPSILON && vx(v) < vx(p) + EPSILON)
    if(vy(v) > vy(p) - EPSILON && vy(v) < vy(p) + EPSILON) {
      GTS_POINT(v)->x = vx(p);
      GTS_POINT(v)->y = vy(p);
    }

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
        space += min_spacing(prevv, nextv);
        prevv = nextv;
      }
      i = i->next;
    }
/*    
    constraint_spacing = constraint_arc_min_spacing(e, origin, &edgev, space);
    if(finite(constraint_spacing) && edgev == tedge_v2(e)) {
      if(space < constraint_spacing) {
        v->flags |= VERTEX_FLAG_RED;
        printf("CONSTRAINT SPACING ADJUSTMENT\n");
        return constraint_spacing;  
      }
    }
*/
  }else{

    /* towards v1 */
    while(i) {
      nextv = edge_routing_prev(e, i);
      if(!(nextv->flags & VERTEX_FLAG_TEMP)) {
        space += min_spacing(prevv, nextv);
        prevv = nextv;
      }
      i = i->prev;
    }
/*
    constraint_spacing = constraint_arc_min_spacing(e, origin, &edgev, space);
    if(finite(constraint_spacing) && edgev == tedge_v1(e)) {
      if(space < constraint_spacing) {
        v->flags |= VERTEX_FLAG_RED;
        printf("CONSTRAINT SPACING ADJUSTMENT\n");
        return constraint_spacing;  
      }
    }
*/
  }


  return space;
}

void
calculate_point_movement3(toporouter_vertex_t *v, GtsVertex *p, toporouter_edge_t *e, gdouble *x, gdouble *y) 
{
  GList *vlist = g_list_find(edge_routing(e), v);
  gdouble force = 0.;
  gdouble v1ms, v2ms, v1d, v2d, v1pd, v2pd, nextspace, prevspace, nextminspace, prevminspace;

  if(v == TOPOROUTER_VERTEX(edge_v1(e)) || v == TOPOROUTER_VERTEX(edge_v2(e))) return;

  v1ms = edge_min_spacing(vlist, e, TOPOROUTER_VERTEX(edge_v1(e)));
  v2ms = edge_min_spacing(vlist, e, TOPOROUTER_VERTEX(edge_v2(e)));
  v1d = gts_point_distance(GTS_POINT(v), GTS_POINT(edge_v1(e)));
  v2d = gts_point_distance(GTS_POINT(v), GTS_POINT(edge_v2(e)));
  
  v1pd = sqrt(pow(v->pullx - vx(edge_v1(e)),2) + pow(v->pully - vy(edge_v1(e)),2));
  v2pd = sqrt(pow(v->pullx - vx(edge_v2(e)),2) + pow(v->pully - vy(edge_v2(e)),2));
  
  nextspace = vlist->next ? gts_point_distance(GTS_POINT(v), GTS_POINT(vlist->next->data)) : v2d;
  prevspace = vlist->prev ? gts_point_distance(GTS_POINT(v), GTS_POINT(vlist->prev->data)) : v1d;
  
  nextminspace = min_spacing(v, (vlist->next?TOPOROUTER_VERTEX(vlist->next->data):tedge_v2(e))) * 0.9;
  prevminspace = min_spacing(v, (vlist->prev?TOPOROUTER_VERTEX(vlist->prev->data):tedge_v1(e))) * 0.9;

  if(v1d <= v1ms) {
    force = (v1ms-v1d);
  }else if(v2d <= v2ms) {
    force = (v2d-v2ms);
  }
  
  if(finite(v->pullx) && finite(v->pully)) {

    if(v1pd < v1d) {
      /* towards v1 */
      force += (v1pd-v1d) * 0.25;
//      if(fabs(force) > v1d-v1ms) 
 //       force = (v1ms-v1d) * 0.9;
      
    }else if(v2pd < v2d) {
      /* towards v2 */
      force += (v2d-v2pd) * 0.25;
 //     if(force > v2d-v2ms) 
 //       force = (v2d-v2ms) * 0.9;

    }

  }
  

  /* sanity check */
  if(force > 0. && force > nextspace - nextminspace) force = nextspace - nextminspace;
  if(force < 0. && fabs(force) > prevspace - prevminspace) force = -(prevspace - prevminspace);
 
//  if(force > EPSILON && force < 1.) force = 1.;
//  if(force < -EPSILON && force > -1.) force = -1.;

  vertex_move_towards_vertex_values(GTS_VERTEX(v), edge_v2(e), force, x, y);

}

void
calculate_point_movement2(toporouter_vertex_t *v, GtsVertex *p, toporouter_edge_t *e, gdouble *x, gdouble *y) 
{
  GList *vlist = g_list_find(edge_routing(e), v);
  gdouble force = 0.;
  gdouble v1ms, v2ms, v1d, v2d, v1pd, v2pd, nextspace, prevspace, nextminspace, prevminspace;

  if(v == TOPOROUTER_VERTEX(edge_v1(e)) || v == TOPOROUTER_VERTEX(edge_v2(e))) return;

  v1ms = edge_min_spacing(vlist, e, TOPOROUTER_VERTEX(edge_v1(e)));
  v2ms = edge_min_spacing(vlist, e, TOPOROUTER_VERTEX(edge_v2(e)));
  v1d = gts_point_distance(GTS_POINT(v), GTS_POINT(edge_v1(e)));
  v2d = gts_point_distance(GTS_POINT(v), GTS_POINT(edge_v2(e)));
  v1pd = gts_point_distance(GTS_POINT(p), GTS_POINT(edge_v1(e)));
  v2pd = gts_point_distance(GTS_POINT(p), GTS_POINT(edge_v2(e)));
  
  nextspace = vlist->next ? gts_point_distance(GTS_POINT(v), GTS_POINT(vlist->next->data)) : v2d;
  prevspace = vlist->prev ? gts_point_distance(GTS_POINT(v), GTS_POINT(vlist->prev->data)) : v1d;

  nextminspace = min_spacing(v, (vlist->next?TOPOROUTER_VERTEX(vlist->next->data):tedge_v2(e))) * 0.9;
  prevminspace = min_spacing(v, (vlist->prev?TOPOROUTER_VERTEX(vlist->prev->data):tedge_v1(e))) * 0.9;

  if(v1d <= v1ms) {
    force = (v1ms-v1d);// * 0.9;
  }else if(v2d <= v2ms) {
    force = (v2d-v2ms);// * 0.9;
  }

  if(v1pd < v1d) {
    /* towards v1 */
    force += (v1pd-v1d) * 0.25;
  }else if(v2pd < v2d) {
    /* towards v2 */
    force += (v2d-v2pd) * 0.25;

  }
  
  /* sanity check */
  if(force > 0. && force > nextspace - nextminspace) force = nextspace - nextminspace;
  if(force < 0. && fabs(force) > prevspace - prevminspace) force = -(prevspace - prevminspace);

//  }
 
//  if(force > EPSILON && force < 1.) force = 1.;
//  if(force < -EPSILON && force > -1.) force = -1.;

  vertex_move_towards_vertex_values(GTS_VERTEX(v), edge_v2(e), force, x, y);

}

void
calculate_point_movement(toporouter_vertex_t *v, GtsVertex *p, toporouter_edge_t *e, gdouble *x, gdouble *y) 
{
  GList *vlist = g_list_find(edge_routing(e), v);
  toporouter_vertex_t *temp;
  gdouble force = 0.;
  gdouble minspace, distance, dm;
  gdouble direction;

  if(v == TOPOROUTER_VERTEX(edge_v1(e)) || v == TOPOROUTER_VERTEX(edge_v2(e))) return;
  
  if(gts_point_distance(GTS_POINT(edge_v2(e)), GTS_POINT(v)) < gts_point_distance(GTS_POINT(edge_v2(e)), GTS_POINT(p))) 
    /* towards v1 */
    direction = 1;
//    force -= gts_point_distance(GTS_POINT(v), GTS_POINT(p)) * k;
  else 
    /* towards v2 */
    direction = -1;
//    force += gts_point_distance(GTS_POINT(v), GTS_POINT(p)) * k;

  /* determine force of neighbors upon v */

  temp = edge_routing_next(e,vlist);
  distance = gts_point_distance(GTS_POINT(v), GTS_POINT(temp));
  minspace = min_spacing(temp,v);
  dm = distance - minspace;
  if(dm < 0.) { 
    if(temp == TOPOROUTER_VERTEX(edge_v2(e)))
      force += dm * 1.0;
    else
      force += dm * 1.0;
  }
  if(direction < 0) {
    force += gts_point_distance(GTS_POINT(v), GTS_POINT(p)) * 0.1;
  }


  temp = edge_routing_prev(e,vlist);
  distance = gts_point_distance(GTS_POINT(v), GTS_POINT(temp));
  minspace = min_spacing(temp,v);
  dm = distance - minspace;
  if(dm < 0.) {
    if(temp == TOPOROUTER_VERTEX(edge_v1(e)))
      force -= dm * 1.1;
    else
      force -= dm * 1.1;
  }
  if(direction > 0) {
    force -= gts_point_distance(GTS_POINT(v), GTS_POINT(p)) * 0.1;
  }


  /* sanity check */
  if(force > 0.)
  if(gts_point_distance(GTS_POINT(v), GTS_POINT(edge_v2(e))) < force) force = gts_point_distance(GTS_POINT(v), GTS_POINT(edge_v2(e)));
  if(force < 0.)
  if(gts_point_distance(GTS_POINT(v), GTS_POINT(edge_v1(e))) < fabs(force)) force = -gts_point_distance(GTS_POINT(v), GTS_POINT(edge_v1(e)));



  vertex_move_towards_vertex_values(GTS_VERTEX(v), edge_v2(e), force, x, y);

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

  if(*x > MIN(x1,x2) && *x < MAX(x1,x2) && *y > MIN(y1,y2) && *y < MAX(y1,y2)) return 1;
  return 0;
}

// returns how much v3 deviates from the line of v1 v2
inline gdouble 
line_deviation(toporouter_vertex_t *v1, toporouter_vertex_t *v2, toporouter_vertex_t *v3) 
{
  gdouble x, y;
  vertex_line_normal_intersection(vx(v1), vy(v1), vx(v2), vy(v2), vx(v3), vy(v3), &x, &y);
  return sqrt(pow(vx(v3)-x,2) + pow(vy(v3)-y,2));
}

guint
vertex_real_check(toporouter_vertex_t *v)
{
  if(v->flags & VERTEX_FLAG_TEMP || v->flags & VERTEX_FLAG_ROUTE) return 0;
  return 1;
}

toporouter_vertex_t *
get_curve_dest2(toporouter_vertex_t *v, toporouter_vertex_t *lastv)
{
  toporouter_vertex_t *nv = v->child, *src = v;

  if(v == lastv) return lastv;

  while(nv && nv != lastv) {
    toporouter_edge_t *e = nv->routingedge;
    toporouter_vertex_t *j = src->child;
    
    if(TOPOROUTER_IS_CONSTRAINT(e)) break;
    
    while(j != nv && j != lastv) {
      gint jwind1, jwind2;
      toporouter_edge_t *je = j->routingedge;
      toporouter_vertex_t *k = j->child;


      jwind1 = vertex_wind(GTS_VERTEX(src), edge_v1(je), GTS_VERTEX(j));
      jwind2 = vertex_wind(GTS_VERTEX(src), edge_v2(je), GTS_VERTEX(j));

      while(k && k != lastv) {
        gint kwind1, kwind2;

        kwind1 = vertex_wind(GTS_VERTEX(src), edge_v1(je), GTS_VERTEX(k));
        kwind2 = vertex_wind(GTS_VERTEX(src), edge_v2(je), GTS_VERTEX(k));
        
        if(jwind1 && kwind1 && jwind1 != kwind1) {
          return v;
        }
        if(jwind2 && kwind2 && jwind2 != kwind2) {
          return v;
        }

        if(k==nv) break;
        k = k->child;
      }

      j = j->child;
    }
    v = nv;
    nv = nv->child;
  }

  if(nv == lastv) return nv;

  return v;
}

toporouter_vertex_t *
get_curve_dest(toporouter_vertex_t *v, toporouter_vertex_t *lastv)
{
  toporouter_vertex_t *nv = v->child, *pv = NULL, *src = v;
  gint pdir=42, dir=42; 

  guint count_neg = 0, count_pos = 0;  

  if(v == lastv) return lastv;

  while(nv) {

    if(nv == lastv) {
      printf("lastv reached\n");
      return nv;
    }
//    if(nv->routingedge) 
//      if(TOPOROUTER_IS_CONSTRAINT(nv->routingedge)) break;
    if(nv && nv->routingedge && count_pos > 2) {

      gint windv1 = vertex_wind(GTS_VERTEX(v), GTS_VERTEX(nv), GTS_VERTEX(edge_v1(nv->routingedge)));

      if(windv1 == pdir) {
        //gint srcwind = vertex_wind(GTS_VERTEX(src), GTS_VERTEX(nv), GTS_VERTEX(edge_v1(i->routingedge)));
        gint srcwind = vertex_wind(GTS_VERTEX(src), GTS_VERTEX(edge_v1(nv->routingedge)), GTS_VERTEX(nv));
        tedge_v1(nv->routingedge)->flags |= VERTEX_FLAG_RED;
        if(srcwind == pdir) return nv;
      }else if(windv1 == -pdir) {
        //          gint srcwind = vertex_wind(GTS_VERTEX(src), GTS_VERTEX(nv), GTS_VERTEX(edge_v2(i->routingedge)));
        gint srcwind = vertex_wind(GTS_VERTEX(src), GTS_VERTEX(edge_v2(nv->routingedge)), GTS_VERTEX(nv));
        tedge_v2(nv->routingedge)->flags |= VERTEX_FLAG_RED;
        if(srcwind == pdir) return nv;
      }

    }

    if(!(nv->flags & VERTEX_FLAG_ROUTE)) goto get_curve_dest_cont;
//    if(!(v->flags & VERTEX_FLAG_ROUTE)) goto get_curve_dest_cont;

    if(pv) {
      gdouble dev;

      dir = vertex_wind(GTS_VERTEX(pv), GTS_VERTEX(v), GTS_VERTEX(nv));

      dev = line_deviation(pv,v,nv);
/*
      if(dev > 100.) {
        if(dir > 0) count_pos += 1;
        if(dir < 0) count_neg += 1;
      }
*/
      if(pdir != 42) {
        if(dir == pdir) count_pos++;
        else count_neg++;
      }

      if(pdir == 42 && dir) pdir = dir;
      else if(dir && dir != pdir) {
        // determine how much the wind of pv differs from the line of v and nv  
 
        if(count_neg > count_pos) {
          guint temp = count_pos;
          count_pos = count_neg;
          count_neg = temp;
          pdir = dir;
        }
 
        if(dev > 100.) break;

        dir = pdir;
      }
  

    }
get_curve_dest_cont:    
/*
    pi = NULL;
    i = src;
    while(i != nv) {

      if(pi && i->routingedge) {

        gint windv1 = vertex_wind(GTS_VERTEX(pi), GTS_VERTEX(i), GTS_VERTEX(edge_v1(i->routingedge)));

        if(windv1 == pdir) {
          //gint srcwind = vertex_wind(GTS_VERTEX(src), GTS_VERTEX(nv), GTS_VERTEX(edge_v1(i->routingedge)));
          gint srcwind = vertex_wind(GTS_VERTEX(src), GTS_VERTEX(edge_v1(i->routingedge)), GTS_VERTEX(i));
          tedge_v1(i->routingedge)->flags |= VERTEX_FLAG_RED;
          if(srcwind && srcwind == pdir) return i;
        }else if(windv1 == -pdir) {
//          gint srcwind = vertex_wind(GTS_VERTEX(src), GTS_VERTEX(nv), GTS_VERTEX(edge_v2(i->routingedge)));
          gint srcwind = vertex_wind(GTS_VERTEX(src), GTS_VERTEX(edge_v2(i->routingedge)), GTS_VERTEX(i));
          tedge_v2(i->routingedge)->flags |= VERTEX_FLAG_RED;
          if(srcwind && srcwind == pdir) return i;
        }

      }
      pi = i;
      i = i->child;
    }
*/
    if(nv->routingedge) 
      if(TOPOROUTER_IS_CONSTRAINT(nv->routingedge)) {
        v = nv;
        break; 
      }

    pdir = dir;
    pv = v;
    v = nv;
    nv = nv->child;
  }

  return v;
}

typedef struct {
  gdouble x, y;
  toporouter_vertex_t *v;
} toporouter_spring_movement_t;



void
spring_embedder(toporouter_t *r) 
{
  guint m;
  GSList *i, *moves = NULL;

  i = r->paths;
  while(i) {
    GSList *j = (GSList *) i->data;

    while(j) {
      toporouter_vertex_t *v = TOPOROUTER_VERTEX(j->data);
      toporouter_edge_t *e = v->routingedge;
      if(v->flags & VERTEX_FLAG_ROUTE && !TOPOROUTER_IS_CONSTRAINT(e)) {
        toporouter_spring_movement_t *move = malloc(sizeof(toporouter_spring_movement_t));
        move->v = v;
        moves = g_slist_prepend(moves, move);
      }
      j = j->next;
    }

    i = i->next;
  }


  for(m=0;m<100;m++) {
//    printf("spring embedder pass %d\n", m);

    i = r->paths;
    while(i) {
      GSList *j = (GSList *) i->data;
      while(j) {
        toporouter_vertex_t *v = TOPOROUTER_VERTEX(j->data);
        v->pullx = v->pully = INFINITY;
        j = j->next;
      }
      i = i->next;
    }


    i = r->paths;
    while(i) {
//      GSList *fulllistj = (GSList *) i->data;
      GSList *j = (GSList *) i->data;
      toporouter_vertex_t *firstv = TOPOROUTER_VERTEX(j->data);

      firstv->child = TOPOROUTER_VERTEX(j->next->data);
//      printf("length = %d\n", g_slist_length(j));

      //j = j->next;
      while(j) {
        toporouter_vertex_t *v = TOPOROUTER_VERTEX(j->data), *dest, *vv, *lastv;
        GSList *k;

        lastv = TOPOROUTER_VERTEX(g_slist_last(j)->data);

//        printf("getting curve dest of %f,%f lastv = %f,%f ", vx(v), vy(v), vx(lastv), vy(lastv));
        dest = get_curve_dest2(v, lastv);

//        printf("dest = %f,%f\n", vx(dest), vy(dest));

//        printf("start pos = %d pos in list = %d\n", g_slist_index(fulllistj, v), g_slist_index(fulllistj, dest));

//        v->cdest = dest;

        k = j;
        vv = TOPOROUTER_VERTEX(k->data);
        while(vv != dest) {
          gdouble x, y, oldd, newd;
          vv = TOPOROUTER_VERTEX(k->data);

          vertex_line_normal_intersection(vx(v), vy(v), vx(dest), vy(dest), vx(vv), vy(vv), &x, &y);
          if(vv->routingedge) {
            if(finite(vv->pullx) && finite(vv->pully)) { 
              oldd = pow(vx(vv) - vv->pullx, 2) + pow(vy(vv) - vv->pully, 2);
              vprojection(x - vx(vv), y - vy(vv), 
                  vx(edge_v2(vv->routingedge)) - vx(vv),
                  vy(edge_v2(vv->routingedge)) - vy(vv),
                  &x,
                  &y);
              x += vx(vv);
              y += vy(vv);

              newd = pow(vx(vv) - x, 2) + pow(vy(vv) - y, 2);

              if(newd > oldd) {
                vv->pullx = x;
                vv->pully = y;
              }
            }else{
              vprojection(x - vx(vv), y - vy(vv), 
                  vx(edge_v2(vv->routingedge)) - vx(vv),
                  vy(edge_v2(vv->routingedge)) - vy(vv),
                  &(vv->pullx),
                  &(vv->pully));
              vv->pullx += vx(vv);
              vv->pully += vy(vv);
            }
          }

          k = k->next;
        }
//        j = k->next;
        j = j->next;

      }
      i = i->next;
    }
    
    i = moves;
    while(i) {
      toporouter_spring_movement_t *move = (toporouter_spring_movement_t *)i->data;
      toporouter_vertex_t *v = move->v;
      toporouter_edge_t *e = v->routingedge;
  
      if(TOPOROUTER_IS_CONSTRAINT(e)) {
        i = i->next;
        continue;
      }

      if(v->flags & VERTEX_FLAG_ROUTE) {
//        GtsVertex *iv;
 
        if(vx(v->child) == vx(v) && vy(v->child) == vy(v)) {
          i = i->next;
          continue;
        }
        if(vx(v->parent) == vx(v) && vy(v->parent) == vy(v)) {
          i = i->next;
          continue;
        }
        
        calculate_point_movement3(v, NULL, e, &(move->x), &(move->y));
//        GTS_POINT(move->v)->x = move->x;
//        GTS_POINT(move->v)->y = move->y;

      }

      i = i->next;
    }
//   /* 
    i = moves;
    while(i) {
      toporouter_spring_movement_t *move = (toporouter_spring_movement_t *)i->data;
  
      GTS_POINT(move->v)->x = move->x;
      GTS_POINT(move->v)->y = move->y;

      i = i->next;
    }
//*/
  }
  
/* 
  {
    int i;
    for(i=0;i<groupcount();i++) {
      char buffer[256];
      sprintf(buffer, "spring2%d.png", i);
      toporouter_draw_surface(r, r->layers[i].surface, buffer, 2048, 2048, 2, NULL, i, NULL);
    }
  }
*/
  for(m=0;m<100;m++) { 
    
    i = moves;
    while(i) {
      toporouter_spring_movement_t *move = (toporouter_spring_movement_t *)i->data;
      toporouter_vertex_t *v = move->v;
      toporouter_edge_t *e = v->routingedge;
  
      if(TOPOROUTER_IS_CONSTRAINT(e)) {
        i = i->next;
        continue;
      }

      if(v->flags & VERTEX_FLAG_ROUTE) {
        GtsVertex *iv;
 
        if(vx(v->child) == vx(v) && vy(v->child) == vy(v)) {
          i = i->next;
          continue;
        }
        if(vx(v->parent) == vx(v) && vy(v->parent) == vy(v)) {
          i = i->next;
          continue;
        }
/*
        if(v->routingedge == NULL) {
          printf("v = %f,%f parent = %f,%f child = %f,%f\n",
              vx(v), vy(v), vx(v->parent), vy(v->parent), vx(v->child), vy(v->child));

        }
*/
        g_assert(v->child);
        g_assert(v->parent);
        g_assert(e);

        if((iv = vertex_intersect(GTS_VERTEX(v->child), GTS_VERTEX(v->parent), edge_v1(e), edge_v2(e)))) {

          calculate_point_movement2(v, iv, e, &(move->x), &(move->y));

          gts_object_destroy(GTS_OBJECT(iv));

        }else{
          gdouble ptv_tv_ntv, ptv_v1_ntv;
            
          g_assert(v->child);
          g_assert(v->parent);
          g_assert(edge_v1(e));
          g_assert(v);

          ptv_tv_ntv = gts_point_distance(GTS_POINT(v->child), GTS_POINT(v)) + 
            gts_point_distance(GTS_POINT(v), GTS_POINT(v->parent));
          ptv_v1_ntv = gts_point_distance(GTS_POINT(v->child), GTS_POINT(edge_v1(e))) + 
            gts_point_distance(GTS_POINT(edge_v1(e)), GTS_POINT(v->parent));

          g_assert(ptv_v1_ntv != ptv_tv_ntv);

          if(ptv_v1_ntv < ptv_tv_ntv) 
            // snap to v1 
            calculate_point_movement2(v, edge_v1(e), e, &(move->x), &(move->y));
          else
            // snap to v2 
            calculate_point_movement2(v, edge_v2(e), e, &(move->x), &(move->y));
          
          

        }
//        GTS_POINT(move->v)->x = move->x;
//        GTS_POINT(move->v)->y = move->y;

      }

      i = i->next;
    }
//   /* 
    i = moves;
    while(i) {
      toporouter_spring_movement_t *move = (toporouter_spring_movement_t *)i->data;
  
      GTS_POINT(move->v)->x = move->x;
      GTS_POINT(move->v)->y = move->y;

      i = i->next;
    }
//*/
  }

  i = moves;
  while(i) {
    free(i->data);

    i = i->next;
  }
  g_slist_free(moves);

}

#define FARFAR_V(x) ((x->next) ? ( (x->next->next) ? ((x->next->next->next) ? TOPOROUTER_VERTEX(x->next->next->next->data) : NULL ) : NULL ) : NULL)
#define FAR_V(x) ((x->next) ? ( (x->next->next) ? TOPOROUTER_VERTEX(x->next->next->data) : NULL ) : NULL)
#define NEXT_V(x) ((x->next) ? TOPOROUTER_VERTEX(x->next->data) : NULL)
#define CUR_V(x) TOPOROUTER_VERTEX(x->data)

#define FARPOINT(x) GTS_POINT(FAR_V(x))
#define NEXTPOINT(x) GTS_POINT(NEXT_V(x))
#define CURPOINT(x) GTS_POINT(CUR_V(x))


toporouter_vertex_t *
prev_lock(GList *j, toporouter_edge_t *e, toporouter_vertex_t *v)
{
  toporouter_vertex_t *pv = j->prev ? TOPOROUTER_VERTEX(j->prev->data) : TOPOROUTER_VERTEX(edge_v1(e));

  gdouble pv_delta = gts_point_distance(GTS_POINT(pv), GTS_POINT(v)) - min_spacing(pv, v);
  gdouble minspacing = min_spacing(pv,v);
  printf("pv_delta = %f min_spacing = %f\n", pv_delta, minspacing);

  if(pv_delta < 1.) {
    

    if(j->prev) return prev_lock(j->prev, e, pv);
    else return TOPOROUTER_VERTEX(edge_v1(e));
//    return TOPOROUTER_VERTEX(edge_v1(e));
  }/*
  else {
    gdouble minspacing = min_spacing(pv,v);
    printf("pv_delta = %f min_spacing = %f\n", pv_delta, minspacing);

  }*/
  return NULL;
}


toporouter_edge_t *
segments_get_other_segment(GtsSegment *s1, GtsSegment *s2)
{
  toporouter_vertex_t *temp = segment_common_vertex(s1, s2); 
  toporouter_vertex_t *v1, *v2;

  v1 = (TOPOROUTER_VERTEX(s1->v1) == temp) ? TOPOROUTER_VERTEX(s1->v2) : TOPOROUTER_VERTEX(s1->v1);
  v2 = (TOPOROUTER_VERTEX(s2->v1) == temp) ? TOPOROUTER_VERTEX(s2->v2) : TOPOROUTER_VERTEX(s2->v1);

  return TOPOROUTER_EDGE(gts_vertices_are_connected(GTS_VERTEX(v1), GTS_VERTEX(v2)));
}

toporouter_vertex_t *
next_lock2(GList *j, toporouter_edge_t *e, toporouter_vertex_t *v)
{
  gdouble minspacing = edge_min_spacing(j,e,TOPOROUTER_VERTEX(edge_v2(e)));
  gdouble space = gts_point_distance(GTS_POINT(v), GTS_POINT(edge_v2(e)));

#ifdef DEBUG_EXPORT
//  printf("nextlock delta = %f space = %f minspacing = %f\n", space - minspacing, space, minspacing);
#endif

//  if(space < minspacing + 50.) return TOPOROUTER_VERTEX(edge_v2(e));
  if(space < minspacing - EPSILON) {
    if(tedge_v2(e)->fakev) return tedge_v2(e)->fakev;
    return tedge_v2(e);
  }
  return NULL;
}

toporouter_vertex_t *
prev_lock2(GList *j, toporouter_edge_t *e, toporouter_vertex_t *v)
{
  gdouble minspacing = edge_min_spacing(j,e,TOPOROUTER_VERTEX(edge_v1(e)));
  gdouble space = gts_point_distance(GTS_POINT(v), GTS_POINT(edge_v1(e)));

#ifdef DEBUG_EXPORT
//  printf("prevlock delta = %f space = %f minspacing = %f\n", space - minspacing, space, minspacing);
#endif

//  if(space < minspacing + 50.) return TOPOROUTER_VERTEX(edge_v1(e));
  if(space < minspacing - EPSILON) {
    if(tedge_v1(e)->fakev) return tedge_v1(e)->fakev;
    return tedge_v1(e);
  }

  return NULL;
}

toporouter_vertex_t *
constraint_arc_lock_test(toporouter_vertex_t *v, toporouter_vertex_t *farv)
{
  
  if(farv != v && farv) {
    toporouter_vertex_t *cv = NULL;

    if(farv->child && TOPOROUTER_IS_CONSTRAINT(farv->child->routingedge)) {
      cv = farv->child;
    }else if(farv->parent && TOPOROUTER_IS_CONSTRAINT(farv->parent->routingedge)) {
      cv = farv->parent;
    }

    if(cv) {
      gdouble r, d, ms;
     
      g_assert(cv->fakev);

      r = (vertex_net_thickness(cv) / 2.) + vertex_net_keepaway(cv);
      d = gts_point_distance(GTS_POINT(cv->fakev), GTS_POINT(v));
      ms = min_spacing(v, farv);

#ifdef DEBUG_EXPORT
      printf("CONSTRAINT ARC LOCK r = %f, d = %f, ms = %f\n", r, d, ms);
      printf("v = %f,%f farv = %f,%f cv = %f,%f\n", vx(v), vy(v), vx(farv), vy(farv), vx(cv), vy(cv));
#endif
      if(d < ms + r) {
#ifdef DEBUG_EXPORT
        printf("returning the fakev\n");
#endif
        return cv->fakev;
      }

    }

  }

  return NULL;
}


/*
gdouble
next_min_spacing(toporouter_vertex_t *v) 
{
  GList *i;
  toporouter_vertex_t *prevv = v, *nextv;
  toporouter_edge_t *el;
  gdouble space = 0.;

  if(!v->routingedge) return INFINITY;
  
  e = v->routingedge;
  i = edge_routing(e);

  while(i) {
    nextv = edge_routing_next(e, i);
    if(!(nextv->flags & VERTEX_FLAG_TEMP)) {
      space += min_spacing(prevv, nextv);
      prevv = nextv;
    }
    i = i->next;
  }

  return space;
}

gdouble
prev_min_spacing(toporouter_vertex_t *v) 
{
  GList *i;
  toporouter_vertex_t *prevv = v, *nextv;
  toporouter_edge_t *el;
  gdouble space = 0.;

  if(!v->routingedge) return INFINITY;
  
  e = v->routingedge;
  i = edge_routing(e);

  while(i) {
    nextv = edge_routing_prev(e, i);
    if(!(nextv->flags & VERTEX_FLAG_TEMP)) {
      space += min_spacing(prevv, nextv);
      prevv = nextv;
    }
    i = i->prev;
  }

  return space;
}
*/
// given two edges, return a third which makes a triangle
toporouter_edge_t *
edges_get_edge(toporouter_edge_t *e1, toporouter_edge_t *e2)
{
  toporouter_vertex_t *a = segment_common_vertex(GTS_SEGMENT(e1), GTS_SEGMENT(e2)); 
  toporouter_vertex_t *v1, *v2;

  if(!a) return NULL;

  if(tedge_v1(e1) == a) v1 = tedge_v2(e1);
  else v1 = tedge_v1(e1);

  if(tedge_v1(e2) == a) v2 = tedge_v2(e2);
  else v2 = tedge_v1(e2);

  return TOPOROUTER_EDGE(gts_vertices_are_connected(GTS_VERTEX(v1), GTS_VERTEX(v2)));
}


toporouter_vertex_t *
constraint_arc_lock2(toporouter_edge_t *e1, toporouter_vertex_t *v)
{
  toporouter_vertex_t *nv = v->child;
  toporouter_edge_t *e2 = nv->routingedge, *ce;

  if(!nv || !e2 || TOPOROUTER_IS_CONSTRAINT(e2)) return NULL;

  ce = edges_get_edge(e1, e2);
  
  if(TOPOROUTER_IS_CONSTRAINT(ce) && edge_routing(ce)) {
    toporouter_vertex_t *cv = TOPOROUTER_VERTEX(edge_routing(ce)->data), *cv2, *internalv;
    toporouter_edge_t *e = NULL;
    gdouble ms, x1, y1, x, y, c1, c2, m1, m2;

    if(v->parent == cv || v->child == cv) return NULL;

    if(cv->child && cv->child->routingedge == e1) { e = e1; cv2 = v; internalv = cv->parent; }
    else if(cv->parent && cv->parent->routingedge == e1) { e = e1; cv2 = v; internalv = cv->child; }
    else if(cv->child && cv->child->routingedge == e2) { e = e2; cv2 = nv; internalv = cv->parent; }
    else if(cv->parent && cv->parent->routingedge == e2) { e = e2; cv2 = nv; internalv = cv->child; }

    if(!e) return NULL;
    
    ms = vertices_min_spacing(TOPOROUTER_VERTEX(segment_common_vertex(GTS_SEGMENT(e), GTS_SEGMENT(ce))), cv2); 

#ifdef DEBUG_EXPORT
    printf("ms %f\n", ms);
#endif

    vertex_move_towards_vertex_values(GTS_VERTEX(internalv), GTS_VERTEX(cv), 
        ms + gts_point_distance(GTS_POINT(cv), GTS_POINT(internalv)), &x1, &y1);

    m1 = cartesian_gradient(vx(cv), vy(cv), x1, y1);
    c1 = (isinf(m1)) ? vx(cv) : vy(cv) - (m1 * vx(cv));

    m2 = cartesian_gradient(vx(v), vy(v), vx(nv), vy(nv));
    c2 = (isinf(m2)) ? vx(v) : vy(v) - (m2 * vx(v));

    if(m1 == m2) return NULL;

    if(isinf(m2))
      x = vx(v);
    else if(isinf(m1))
      x = vx(cv);
    else
      x = (c2 - c1) / (m1 - m2);

    if(x > MIN(x1,vx(cv)) && x < MAX(x1,vx(cv))) {
      y = (isinf(m1)) ? (m2 * x) + c2 : (m1 * x) + c1;
      
      if(y > MIN(y1,vy(cv)) && y < MAX(y1,vy(cv))) {
#ifdef DEBUG_EXPORT
        printf("xy = %f,%f x1y1 = %f,%f cv = %f,%f v = %f,%f nv = %f,%f\n", 
            x, y, x1, y1, vx(cv), vy(cv), vx(v), vy(v), vx(nv), vy(nv));
#endif
        return cv->fakev;
      }

    }


  }

  return NULL;
}

toporouter_vertex_t *
constraint_arc_lock(GList *j, toporouter_edge_t *e, toporouter_vertex_t *v)
{
  toporouter_vertex_t *test = NULL;

  test = constraint_arc_lock_test(v, TOPOROUTER_VERTEX(g_list_last(j)->data));

  if(test) return test;

  test = constraint_arc_lock_test(v, TOPOROUTER_VERTEX(g_list_first(j)->data));

  return test;
}

toporouter_vertex_t *
next_lock(GList *j, toporouter_edge_t *e, toporouter_vertex_t *v)
{
  toporouter_vertex_t *nv = j->next ? TOPOROUTER_VERTEX(j->next->data) : TOPOROUTER_VERTEX(edge_v2(e));

  gdouble nv_delta = gts_point_distance(GTS_POINT(nv), GTS_POINT(v)) - min_spacing(nv, v);
  gdouble minspacing = min_spacing(nv,v);
  printf("nv_delta = %f min_spacing = %f\n", nv_delta, minspacing);

  if(nv_delta < 1.) {
    
    
    if(j->next) return next_lock(j->next, e, nv);
    else return TOPOROUTER_VERTEX(edge_v2(e));
//    return TOPOROUTER_VERTEX(edge_v2(e));
  }/*
  else{
    gdouble minspacing = min_spacing(nv,v);
    printf("nv_delta = %f min_spacing = %f\n", nv_delta, minspacing);

  }*/
  return NULL;
}




/* check cut across triangle from prev to v for normal intersection with
 * vertex shared by routing edges of prev and v 
 * If the length of that normal doesn't clear min_spacing, return an arc
 * about that vertex
 */ 
toporouter_vertex_t *
check_triangle_cut_clearance(toporouter_vertex_t *prev, toporouter_vertex_t *v) 
{
  GtsEdge *e = GTS_EDGE(v->routingedge);
  GtsEdge *preve = GTS_EDGE(prev->routingedge);
  toporouter_vertex_t *a; 
  gdouble x, y, m1, m2, c2, c1, len, prevspacing, vspacing;

//  printf("checking triangle cut clearance... pv = %f,%f v = %f,%f\n",
//      vx(prev), vy(prev), vx(v), vy(v));

  if(!e) return NULL;

  if(!preve) {
    preve = GTS_EDGE(gts_vertices_are_connected(edge_v1(e), GTS_VERTEX(prev)));
    if(!preve) {
      preve = GTS_EDGE(gts_vertices_are_connected(edge_v2(e), GTS_VERTEX(prev)));
      if(!preve) return NULL;
    }
  }
  
  if(!e) {
    e = GTS_EDGE(gts_vertices_are_connected(edge_v1(preve), GTS_VERTEX(v)));
    if(!e) {
      e = GTS_EDGE(gts_vertices_are_connected(edge_v2(preve), GTS_VERTEX(v)));
      if(!e) return NULL;
    }
  }

  a = segment_common_vertex(GTS_SEGMENT(e), GTS_SEGMENT(preve)); 
  g_assert(a);
  
//  printf("a = %f,%f\n", vx(a), vy(a));

  m1 = cartesian_gradient(vx(v), vy(v), vx(prev), vy(prev));
  m2 = perpendicular_gradient(m1);
  c2 = (isinf(m2)) ? vx(a) : vy(a) - (m2 * vx(a));
  c1 = (isinf(m1)) ? vx(v) : vy(v) - (m1 * vx(v));

  if(isinf(m2))
    x = vx(a);
  else if(isinf(m1))
    x = vx(v);
  else
    x = (c2 - c1) / (m1 - m2);

  y = (isinf(m2)) ? vy(v) : (m2 * x) + c2;

//  printf("x = %f y = %f\n", x, y);

  if(epsilon_equals(x,vx(v)) && epsilon_equals(y,vy(v))) return NULL;
  if(epsilon_equals(x,vx(prev)) && epsilon_equals(y,vy(prev))) return NULL;

  if(x >= MIN(vx(v),vx(prev)) && x <= MAX(vx(v),vx(prev)) && y >= MIN(vy(v),vy(prev)) && y <= MAX(vy(v),vy(prev))) { 

//  if(c1check > c1 - EPSILON && c1check < c1 + EPSILON) { 

    len = sqrt(pow(vx(a) - x, 2) + pow(vy(a) - y, 2));
    
    g_assert(e);
    g_assert(preve);

    vspacing = edge_min_spacing(g_list_find(edge_routing(TOPOROUTER_EDGE(e)), v), TOPOROUTER_EDGE(e), a);
    prevspacing = edge_min_spacing(g_list_find(edge_routing(TOPOROUTER_EDGE(preve)), prev), TOPOROUTER_EDGE(preve), a);
   
#ifdef DEBUG_EXPORT
    printf("x = %f y = %f len = %f vspacing = %f prevspacing = %f\n", x, y, len, vspacing, prevspacing);
#endif    
    if(finite(vspacing))
      if(len < vspacing) return a;

    if(finite(prevspacing))
        if(len < prevspacing) return a;
  }
//  printf("failed c1check\n");
  return NULL;
}

#define PATH_NEXT_VERTEX_TYPE_ERROR      0
#define PATH_NEXT_VERTEX_TYPE_CONSTRAINT 1
#define PATH_NEXT_VERTEX_TYPE_ARC        2
#define PATH_NEXT_VERTEX_TYPE_TERM       3

guint
path_next_vertex(toporouter_t *r, toporouter_vertex_t *prev, GSList **path, toporouter_vertex_t **vertex, toporouter_vertex_t **centre)
{
//  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  GSList *i = *path;
  GList *j = NULL;

  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);
    toporouter_vertex_t *incident;
    toporouter_edge_t *e = v->routingedge;

    j = NULL;

    if(TOPOROUTER_IS_CONSTRAINT(e)) {
      *centre = v->fakev;
      *vertex = v;
      *path = i->next; 
#ifdef DEBUG_EXPORT
      printf("CONSTRAINT ARC\n");
#endif
      return PATH_NEXT_VERTEX_TYPE_CONSTRAINT;

    }
   
    if(e) {
      toporouter_vertex_t *lockv;
//      /*
      if((lockv = constraint_arc_lock2(e, v))) {
        *vertex = v;
        *centre = lockv;
        *path = i->next;
#ifdef DEBUG_EXPORT
        printf("GOOD ARC LOCK for %f,%f with c %f,%f\n", vx(v), vy(v), vx(lockv), vy(lockv));
        v->flags |= VERTEX_FLAG_RED;
        lockv->flags |= VERTEX_FLAG_GREEN;
#endif      
        return PATH_NEXT_VERTEX_TYPE_ARC;
      }//*/
    }

    /* check cut across triangle from prev to v for normal intersection with
     * vertex shared by routing edges of prev and v 
     * If the length of that normal doesn't clear min_spacing, return an arc
     * about that vertex
     */ 
    if((incident = check_triangle_cut_clearance(prev, v))) {
      *vertex = v;
      *centre = incident;
      *path = i->next;
#ifdef DEBUG_EXPORT      
      printf("INCIDENT ARC %f,%f\n", vx(*centre), vy(*centre));
#endif     
      return PATH_NEXT_VERTEX_TYPE_ARC;
    }    

    if(e) {

      j = g_list_find(edge_routing(e), v);

      if(j) {
//        if(TOPOROUTER_VERTEX(j->data) == v) {
//          int winddir = vertex_wind(GTS_VERTEX(prev), GTS_VERTEX(v), GTS_VERTEX(NEXT_V(i)));
//          int v1winddir = vertex_wind(GTS_VERTEX(prev), GTS_VERTEX(v), edge_v1(e));

      toporouter_vertex_t *lockv;

//          if(winddir == v1winddir || winddir == 0) {
            /* towards v1 */
            lockv = prev_lock2(j, e, v);
            if(lockv) {
              *vertex = v;
              *centre = lockv;
              *path = i->next;
              return PATH_NEXT_VERTEX_TYPE_ARC;
            }

//          }
          
//          if(winddir != v1winddir || winddir == 0){
            /* towards v2 */
            lockv = next_lock2(j, e, v);
            if(lockv) {
              *vertex = v;
              *centre = lockv;
              *path = i->next;
              return PATH_NEXT_VERTEX_TYPE_ARC;
            }

//          }


//        }
      }else{
        printf("ERROR: did not find vertex in its routing edge\n");
        return PATH_NEXT_VERTEX_TYPE_ERROR;
      }
    }
//path_next_vertex_continue:    
    prev = v;
    i = i->next;
#ifdef DEBUG_EXPORT    
    printf("path_next_vertex_continue;\n");
#endif    
  }

  *vertex = prev;
  *centre = NULL;
  *path = NULL;
  return PATH_NEXT_VERTEX_TYPE_TERM;
}


void
print_toporouter_arc(toporouter_arc_t *arc)
{
//  GSList *i = arc->vs;

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

gint
clearance_list_compare(gconstpointer a, gconstpointer b, gpointer user_data)
{
  gdouble ax, ay, bx, by, cx, cy, dx, dy;
  gdouble da, db;
  gdouble *line = (gdouble *) user_data;
  toporouter_clearance_t *ca = TOPOROUTER_CLEARANCE(a), *cb = TOPOROUTER_CLEARANCE(b);

  if(TOPOROUTER_IS_VERTEX(GTS_OBJECT(ca->data))) {
    ax = vx(ca->data); ay = vy(ca->data);
  }else{
    g_assert(TOPOROUTER_IS_ARC(GTS_OBJECT(ca->data)));
    ax = vx(TOPOROUTER_ARC(ca->data)->centre); ay = vy(TOPOROUTER_ARC(ca->data)->centre);
  }

  if(TOPOROUTER_IS_VERTEX(GTS_OBJECT(cb->data))) {
    bx = vx(cb->data); by = vy(cb->data);
  }else{
    g_assert(TOPOROUTER_IS_ARC(GTS_OBJECT(cb->data)));
    bx = vx(TOPOROUTER_ARC(cb->data)->centre); by = vy(TOPOROUTER_ARC(cb->data)->centre);
  }
  
  vertex_line_normal_intersection(line[0], line[1], line[2], line[3], ax, ay, &cx, &cy);
  vertex_line_normal_intersection(line[0], line[1], line[2], line[3], bx, by, &dx, &dy);

  da = pow(line[0]-cx,2)+pow(line[1]-cy,2);
  db = pow(line[0]-dx,2)+pow(line[1]-dy,2);

  if(da<db) return -1;
  if(db<da) return 1;
  return 0;
}
  
void
toporouter_arc_remove(toporouter_oproute_t *oproute, toporouter_arc_t *arc)
{
  /*
  GList *list = g_list_find(oproute->arcs, arc), *i = arc->clearance, **clearance;
  gdouble linedata[4];

  if(list->prev) {
    toporouter_arc_t *parc = TOPOROUTER_ARC(list->prev->data);
    linedata[0] = parc->x1;
    linedata[1] = parc->y1;
  }else{
    linedata[0] = vx(oproute->term1);
    linedata[1] = vy(oproute->term1);
  }
  if(list->next) {
    toporouter_arc_t *narc = TOPOROUTER_ARC(list->next->data);
    linedata[2] = narc->x0;
    linedata[3] = narc->y0;
  }else{
    linedata[2] = vx(oproute->term2);
    linedata[3] = vy(oproute->term2);
  }

  if(list->prev) clearance = &(TOPOROUTER_ARC(list->prev->data)->clearance);
  else clearance = &(oproute->clearance);

  printf("\tinserting %d into new clearance list\n", g_list_length(i));

  while(i) {
    *clearance = g_list_insert_sorted_with_data(*clearance, i->data, clearance_list_compare, &linedata);
    i = i->next;
  }
*/
  oproute->arcs = g_list_remove(oproute->arcs, arc);

  if(arc->v) arc->v->arc = NULL;

//  gts_object_destroy(GTS_OBJECT(arc));
}


toporouter_arc_t *
toporouter_arc_new(toporouter_oproute_t *oproute, toporouter_vertex_t *v, toporouter_vertex_t *centre, gdouble r, gint dir)
{
  toporouter_arc_t *arc = TOPOROUTER_ARC(gts_object_new(GTS_OBJECT_CLASS(toporouter_arc_class())));
  arc->centre = centre;
  arc->v = v;
  arc->r = r;
  arc->dir = dir;

  if(v) arc->v->arc = arc;
  arc->oproute = oproute;

  arc->clearance = NULL;

  return arc;
}

/* fixes dodgey thick traces coming out of small pads */
void
fix_oproute(toporouter_oproute_t *oproute) 
{
  GList *i = oproute->arcs;
  toporouter_arc_t *arc = NULL;

  /* get first arc and determine if it is attached to a constraint */
  if(i) {
    arc = TOPOROUTER_ARC(i->data);
    if(TOPOROUTER_IS_CONSTRAINT(arc->v->routingedge)) 
      if(vertex_bbox(TOPOROUTER_VERTEX(edge_v1(arc->v->routingedge))) == vertex_bbox(oproute->term1)) {
      /* remove all subsequent arcs attached to v1 or v2 of routingedge and
       * recalculate wind dir */
      guint recalculate = 0;

      i = i->next;
      while(i) {
        toporouter_arc_t *temparc = TOPOROUTER_ARC(i->data);
        if(temparc->centre == TOPOROUTER_VERTEX(edge_v1(arc->v->routingedge)) || 
            temparc->centre == TOPOROUTER_VERTEX(edge_v2(arc->v->routingedge))) {
          
          
//          arc_remove(oproute, temparc);
          oproute->arcs = g_list_remove(oproute->arcs, temparc);
          
//          recalculate = 1;
        }else{
          break;
        }
        i = i->next;
      }


      if(recalculate) {
        gint winddir;
        if(i) {
          toporouter_arc_t *temparc = TOPOROUTER_ARC(i->data);
          winddir = vertex_wind(GTS_VERTEX(oproute->term1), GTS_VERTEX(arc->v), GTS_VERTEX(temparc->v));
        }else{
          winddir = vertex_wind(GTS_VERTEX(oproute->term1), GTS_VERTEX(arc->v), GTS_VERTEX(oproute->term2));
        }

        if(winddir != arc->dir) {
          arc->dir = winddir;
          vertex_move_towards_vertex(GTS_VERTEX(arc->centre), GTS_VERTEX(arc->v), 2. * arc->r);
        }
      }
    }
  }
  
  /* get last arc and determine if it is attached to a constraint */
  i = g_list_last(oproute->arcs);
  if(i) {
    arc = TOPOROUTER_ARC(i->data);
    if(TOPOROUTER_IS_CONSTRAINT(arc->v->routingedge)) 
      if(vertex_bbox(TOPOROUTER_VERTEX(edge_v1(arc->v->routingedge))) == vertex_bbox(oproute->term2)) {
      /* remove all previous arcs attached to v1 or v2 of routingedge and
       * recalculate wind dir */
      guint recalculate = 0;

      i = i->prev;
      while(i) {
        toporouter_arc_t *temparc = TOPOROUTER_ARC(i->data);
        if(temparc->centre == TOPOROUTER_VERTEX(edge_v1(arc->v->routingedge)) || 
            temparc->centre == TOPOROUTER_VERTEX(edge_v2(arc->v->routingedge))) {
          oproute->arcs = g_list_remove(oproute->arcs, temparc);
//          arc_remove(oproute, temparc);

//          recalculate = 1;
        }else{
          break;
        }
        i = i->prev;
      }


      if(recalculate) {
        gint winddir;
        if(i) {
          toporouter_arc_t *temparc = TOPOROUTER_ARC(i->data);
          winddir = -vertex_wind(GTS_VERTEX(oproute->term2), GTS_VERTEX(arc->v), GTS_VERTEX(temparc->v));
        }else{
          winddir = -vertex_wind(GTS_VERTEX(oproute->term2), GTS_VERTEX(arc->v), GTS_VERTEX(oproute->term1));
        }

        if(winddir != arc->dir) {
          arc->dir = winddir;
          vertex_move_towards_vertex(GTS_VERTEX(arc->centre), GTS_VERTEX(arc->v), 2. * arc->r);
        }
      }
    }
  }


}

void
path_set_oproute(GSList *path, toporouter_oproute_t *oproute)
{
  while(path) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(path->data);

    if(v->flags & VERTEX_FLAG_ROUTE) 
      v->oproute = oproute;

    path = path->next;
  }
}

toporouter_oproute_t *
optimize_path(toporouter_t *r, GSList *path) 
{
  GSList *j = path;
  toporouter_vertex_t *pv = NULL, *v = NULL, *centre = NULL, *pcentre = NULL;
  toporouter_oproute_t *oproute = malloc(sizeof(toporouter_oproute_t)); 
  toporouter_arc_t *lastarc = NULL;

#ifdef DEBUG_EXPORT
  printf("EXPORTING PATH length %d:\n", g_slist_length(path));
#endif
  TOPOROUTER_VERTEX(g_slist_nth_data(j,g_slist_length(j)-1))->child = NULL;
  TOPOROUTER_VERTEX(g_slist_nth_data(j,g_slist_length(j)-1))->parent = TOPOROUTER_VERTEX(g_slist_nth_data(j,g_slist_length(j)-2));

  pv = TOPOROUTER_VERTEX(j->data);
  pv->parent = NULL;
  pv->child = TOPOROUTER_VERTEX(j->next->data);
  
  oproute->term1 = pv;
  oproute->arcs = NULL;
  oproute->style = vertex_bbox(pv)->cluster->style;
  oproute->netlist = vertex_bbox(pv)->cluster->netlist;
  oproute->layergroup = vz(pv);
  oproute->path = path;
  oproute->clearance = NULL;
  oproute->adj = NULL;
  oproute->serp = NULL;
  //  printf("TERM: "); print_vertex(pv); printf("\n");

  path_set_oproute(path, oproute);

  j = j->next;

  while(j) {

    guint ret = path_next_vertex(r, pv, &j, &v, &centre);
    
    switch(ret) {
      case PATH_NEXT_VERTEX_TYPE_ERROR:
        printf("ERROR: optimize route vertex error\n");
        return NULL;
      case PATH_NEXT_VERTEX_TYPE_CONSTRAINT:
//        printf("CONSTRAINT: ");
        break;
      case PATH_NEXT_VERTEX_TYPE_ARC:
//        printf("ARC: ");
        break;
      case PATH_NEXT_VERTEX_TYPE_TERM:
//        printf("TERM: "); print_vertex(v); printf("\n");
        goto oproute_finish;
      default:
        printf("ERROR: optimize route default error\n");
        return NULL;
    }

    if(centre) {
      if(pcentre != centre) {
        gdouble rad; 
        GList *templist = NULL;
        
        if(v->routingedge) templist = g_list_find(edge_routing(v->routingedge), v);

        if(templist) {
          rad = (ret == PATH_NEXT_VERTEX_TYPE_CONSTRAINT) ? 
              gts_point_distance(GTS_POINT(v), GTS_POINT(centre)) :
              edge_min_spacing(templist, v->routingedge, centre);

        }else{
          toporouter_vertex_t *vv = TOPOROUTER_VERTEX(g_slist_nth(path, g_slist_length(path)-2)->data);
          rad = edge_min_spacing(g_list_find(edge_routing(vv->routingedge), vv), vv->routingedge, centre);
    
          if(ret == PATH_NEXT_VERTEX_TYPE_CONSTRAINT){
            rad = gts_point_distance(GTS_POINT(v), GTS_POINT(centre));
          }

        }
#ifdef DEBUG_EXPORT
        printf("new arc with rad = %f centre = %f,%f \n", rad, vx(centre), vy(centre));
#endif

        lastarc = toporouter_arc_new(oproute, v, centre, rad, vertex_wind(GTS_VERTEX(pv), GTS_VERTEX(v), GTS_VERTEX(centre)));

        oproute->arcs = g_list_append(oproute->arcs, lastarc);
      }else{

        if(gts_point_distance2(GTS_POINT(lastarc->v), GTS_POINT(centre)) > gts_point_distance2(GTS_POINT(v), GTS_POINT(centre))) {
          lastarc->v->arc = NULL;
          
          lastarc->v = v;
          v->arc = lastarc;
        }

      }

//      if(lastarc->vs) g_slist_free(lastarc->vs);

//      lastarc->vs = g_slist_prepend(NULL, v);
    }

    pcentre = centre;
    pv = v;
  }

oproute_finish:  
  oproute->term2 = v;
  fix_oproute(oproute);
  return oproute;
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

void
export_pcb_drawline(guint layer, guint x0, guint y0, guint x1, guint y1, guint thickness, guint keepaway) 
{

  LineTypePtr line;
  line = CreateDrawnLineOnLayer( LAYER_PTR(layer), x0, y0, x1, y1, 
      thickness, keepaway, 
      MakeFlags (AUTOFLAG | (TEST_FLAG (CLEARNEWFLAG, PCB) ? CLEARLINEFLAG : 0)));

  AddObjectToCreateUndoList (LINE_TYPE, LAYER_PTR(layer), line, line);

}

void
export_pcb_drawarc(guint layer, toporouter_arc_t *a, guint thickness, guint keepaway) 
{
  gdouble sa, da;
  gdouble x0, y0, x1, y1;
  ArcTypePtr arc;
  
  sa = coord_xangle(a->x0, a->y0, vx(a->centre), vy(a->centre)) * 180. / M_PI;
  
  x0 = a->x0 - vx(a->centre);
  x1 = a->x1 - vx(a->centre);
  y0 = a->y0 - vy(a->centre);
  y1 = a->y1 - vy(a->centre);

  da = fabs( acos( ((x0*x1)+(y0*y1)) / (sqrt(pow(x0,2)+pow(y0,2))*sqrt(pow(x1,2)+pow(y1,2))) ) ) * 180. / M_PI * (-a->dir);

  if(da < 1. && da > -1.) return;

  arc = CreateNewArcOnLayer(LAYER_PTR(layer), vx(a->centre), vy(a->centre), a->r, a->r,
    sa, da, thickness, keepaway, 
    MakeFlags( AUTOFLAG | (TEST_FLAG (CLEARNEWFLAG, PCB) ? CLEARLINEFLAG : 0)));

  AddObjectToCreateUndoList( ARC_TYPE, LAYER_PTR(layer), arc, arc);

}

void
calculate_term_to_arc(toporouter_vertex_t *v, toporouter_arc_t *arc, guint dir, guint layer)
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
//    export_pcb_drawline(layer, vx(v), vy(v), a0x, a0y, thickness, keepaway);
    if(!dir) {
      arc->x0 = a0x; 
      arc->y0 = a0y;
    }else{
      arc->x1 = a0x; 
      arc->y1 = a0y;
//      export_pcb_drawarc(layer, arc, thickness, keepaway);
    }
  }else{
//    export_pcb_drawline(layer, vx(v), vy(v), a1x, a1y, thickness, keepaway);
    if(!dir) {
      arc->x0 = a1x; 
      arc->y0 = a1y;
    }else{
      arc->x1 = a1x; 
      arc->y1 = a1y;
//      export_pcb_drawarc(layer, arc, thickness, keepaway);
    }
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

void
calculate_arc_to_arc(toporouter_t *ar, toporouter_arc_t *parc, toporouter_arc_t *arc, guint layer)
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
#ifdef DEBUG_EXPORT    
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
#endif
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
#ifdef DEBUG_EXPORT   
    printf("TWIST:\n");
      printf("theta = %f a = %f b = %f r = %f d = %f po = %f\n", theta, a, b, bigr->r + smallr->r,
          gts_point_distance(GTS_POINT(bigr->centre), GTS_POINT(smallr->centre)),
          (bigr->r+smallr->r) / gts_point_distance(GTS_POINT(bigr->centre), GTS_POINT(smallr->centre)));

      printf("bigr centre = %f,%f smallr centre = %f,%f\n\n", vx(bigr->centre), vy(bigr->centre), 
          vx(smallr->centre), vy(smallr->centre));
#endif      
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

}

void
calculate_oproute(toporouter_t *ar, toporouter_oproute_t *oproute)
{
  guint layer = PCB->LayerGroups.Entries[oproute->layergroup][0];   
  GList *arcs = oproute->arcs;
  toporouter_arc_t *arc, *parc = NULL;

  if(!arcs) {
    return;
  }

  calculate_term_to_arc(oproute->term1, TOPOROUTER_ARC(arcs->data), 0, layer);

  while(arcs) {
    arc = TOPOROUTER_ARC(arcs->data);

    if(parc && arc) {
      calculate_arc_to_arc(ar, parc, arc, layer);
    }

    parc = arc;
    arcs = arcs->next;
  }

  calculate_term_to_arc(oproute->term2, arc, 1, layer);

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
    export_pcb_drawline(layer, vx(oproute->term1), vy(oproute->term1), vx(oproute->term2), vy(oproute->term2), thickness, keepaway);
    return;
  }


//  calculate_term_to_arc(oproute->term1, TOPOROUTER_ARC(arcs->data), 0, layer);

  while(arcs) {
    arc = TOPOROUTER_ARC(arcs->data);

    if(parc && arc) {
      export_pcb_drawarc(layer, parc, thickness, keepaway);
      export_pcb_drawline(layer, parc->x1, parc->y1, arc->x0, arc->y0, thickness, keepaway);
    }else if(!parc) {
      export_pcb_drawline(layer, vx(oproute->term1), vy(oproute->term1), arc->x0, arc->y0, thickness, keepaway);
    }

    parc = arc;
    arcs = arcs->next;
  }
  export_pcb_drawarc(layer, arc, thickness, keepaway);
  export_pcb_drawline(layer, arc->x1, arc->y1, vx(oproute->term2), vy(oproute->term2), thickness, keepaway);

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
clean_oproute_arcs(toporouter_oproute_t *oproute)
{
  GList *i = oproute->arcs;
  toporouter_vertex_t *pv = oproute->term1, *v = NULL, *nv = NULL;
  GSList *remlist = NULL, *j;

  i = oproute->arcs;
  while(i) {
    toporouter_arc_t *arc = (toporouter_arc_t *)i->data;
    gint wind;

    v = arc->v;

    if(i->next) nv = ((toporouter_arc_t *)i->next->data)->v;
    else nv = oproute->term2;
    
    wind = vertex_wind(GTS_VERTEX(pv), GTS_VERTEX(arc->v), GTS_VERTEX(nv));


    //arc->dir = vertex_wind(GTS_VERTEX(pv), GTS_VERTEX(arc->v), GTS_VERTEX(v));

    if(wind != arc->dir) {
      remlist = g_slist_prepend(remlist, arc);
    }

    pv = v;
    i = i->next;
  }


  j = remlist;
  while(j) {
    oproute->arcs = g_list_remove(oproute->arcs, j->data);
    j = j->next;
  }

  g_slist_free(remlist);

}

void
fix_overshoot_oproute_arcs(toporouter_oproute_t *oproute)
{
  GList *i = oproute->arcs;
  GSList *remlist = NULL, *j, *fliplist = NULL;
  toporouter_arc_t *parc = NULL;

  i = oproute->arcs;
  while(i) {
    toporouter_arc_t *arc = (toporouter_arc_t *)i->data;

    if(oproute->serp && g_list_find(oproute->serp->arcs, arc)) {
      i = i->next;
      continue;
    }

    if(parc && arc) {
      gdouble theta, a, b, bx, by, a0x, a0y, a1x, a1y, m, preva, prevb;
      gint winddir;
      toporouter_arc_t *bigr, *smallr;

      if(parc->r > arc->r) {
        bigr = parc; smallr = arc;
      }else{
        bigr = arc; smallr = parc;
      }

      g_assert(bigr != smallr);

      m = perpendicular_gradient(point_gradient(GTS_POINT(bigr->centre), GTS_POINT(smallr->centre)));


      if(parc->dir == arc->dir) {
        //export_arc_straight:
#ifdef DEBUG_EXPORT
        printf("EXAMINING OVERSHOOT ON STRAIGHT ARC\n");
#endif 
        theta = acos((bigr->r - smallr->r) / gts_point_distance(GTS_POINT(bigr->centre), GTS_POINT(smallr->centre)));
        a = bigr->r * sin(theta);
        b = bigr->r * cos(theta);

#ifdef DEBUG_EXPORT
        printf("big->r %f small->r %f d %f a %f b %f\n",
          bigr->r, smallr->r, gts_point_distance(GTS_POINT(bigr->centre), GTS_POINT(smallr->centre)), a, b);
#endif 

        point_from_point_to_point(bigr->centre, smallr->centre, b, &bx, &by);

        coords_on_line(bx, by, m, a, &a0x, &a0y, &a1x, &a1y);

        winddir = coord_wind(vx(smallr->centre), vy(smallr->centre), a0x, a0y, vx(bigr->centre), vy(bigr->centre));

        arc_ortho_projections(parc, arc, &prevb, &preva);
       
#ifdef DEBUG_EXPORT
        if(!winddir) {
          printf(" colinear wind with smallr->c %f,%f a0 %f,%f bigr->c %f,%f\n", 
            vx(smallr->centre), vy(smallr->centre), a0x, a0y, vx(bigr->centre), vy(bigr->centre));
        }
#endif 

        g_assert(winddir);

        if(bigr==parc) winddir = -winddir;

        if(winddir == bigr->dir) {
          if(bigr==arc) {
          }else{
            gint wind1, wind2;

            wind1 = coord_wind(vx(bigr->centre), vy(bigr->centre), bigr->x0, bigr->y0, a0x, a0y);
            wind2 = coord_wind(vx(bigr->centre), vy(bigr->centre), a0x, a0y, vx(smallr->centre), vy(smallr->centre));

            if(wind1 != wind2) {
#ifdef DEBUG_EXPORT          
              printf("WARNING: BS TWIST DETECTED %f,%f %f,%f\n", vx(bigr->centre), vy(bigr->centre), vx(smallr->centre), vy(smallr->centre));
#endif
//              if(smallr->centre->flags & VERTEX_FLAG_FAKE) fliplist = g_slist_prepend(fliplist, smallr);
              remlist = g_slist_prepend(remlist, bigr);    
            }
          }
        }else{
          if(bigr==arc) {
          }else{
            gint wind1, wind2;

            wind1 = coord_wind(vx(bigr->centre), vy(bigr->centre), bigr->x0, bigr->y0, a1x, a1y);
            wind2 = coord_wind(vx(bigr->centre), vy(bigr->centre), a1x, a1y, vx(smallr->centre), vy(smallr->centre));

            if(wind1 != wind2) {
#ifdef DEBUG_EXPORT          
              printf("WARNING: BS TWIST DETECTED %f,%f %f,%f\n", vx(bigr->centre), vy(bigr->centre), vx(smallr->centre), vy(smallr->centre));
#endif
//              if(smallr->centre->flags & VERTEX_FLAG_FAKE) fliplist = g_slist_prepend(fliplist, smallr);
              remlist = g_slist_prepend(remlist, bigr);    
            }
          }
        }

        a = smallr->r * sin(theta);
        b = smallr->r * cos(theta);

        point_from_point_to_point(smallr->centre, bigr->centre, -b, &bx, &by);

        coords_on_line(bx, by, m, a, &a0x, &a0y, &a1x, &a1y);

        if(winddir == bigr->dir) {
          if(bigr==arc) {
            gint wind1, wind2;

            wind1 = coord_wind(vx(smallr->centre), vy(smallr->centre), smallr->x0, smallr->y0, a0x, a0y);
            wind2 = coord_wind(vx(smallr->centre), vy(smallr->centre), a0x, a0y, vx(bigr->centre), vy(bigr->centre));

            if(wind1 != wind2) {
#ifdef DEBUG_EXPORT          
              printf("WARNING: SB TWIST DETECTED %f,%f %f,%f\n", vx(bigr->centre), vy(bigr->centre), vx(smallr->centre), vy(smallr->centre));
#endif 
//              if(smallr->centre->flags & VERTEX_FLAG_FAKE) fliplist = g_slist_prepend(fliplist, smallr);
              remlist = g_slist_prepend(remlist, smallr);    
            }
          }else{
          }
        }else{
          if(bigr==arc) {
            gint wind1, wind2;

            wind1 = coord_wind(vx(smallr->centre), vy(smallr->centre), smallr->x0, smallr->y0, a1x, a1y);
            wind2 = coord_wind(vx(smallr->centre), vy(smallr->centre), a1x, a1y, vx(bigr->centre), vy(bigr->centre));

            if(wind1 != wind2) {
#ifdef DEBUG_EXPORT          
              printf("WARNING: SB TWIST DETECTED %f,%f %f,%f\n", vx(bigr->centre), vy(bigr->centre), vx(smallr->centre), vy(smallr->centre));
#endif
//              if(smallr->centre->flags & VERTEX_FLAG_FAKE) fliplist = g_slist_prepend(fliplist, smallr);
              remlist = g_slist_prepend(remlist, smallr);    
            }
          }else{
          }
        }

      }else{
///*
        //export_arc_twist:    

        theta = acos((bigr->r + smallr->r) / gts_point_distance(GTS_POINT(bigr->centre), GTS_POINT(smallr->centre)));
        a = bigr->r * sin(theta);
        b = bigr->r * cos(theta);

        point_from_point_to_point(bigr->centre, smallr->centre, b, &bx, &by);

        coords_on_line(bx, by, m, a, &a0x, &a0y, &a1x, &a1y);

        winddir = coord_wind(vx(smallr->centre), vy(smallr->centre), a0x, a0y, vx(bigr->centre), vy(bigr->centre));
#ifdef DEBUG_EXPORT   
        printf("TWIST:\n");
        printf("theta = %f a = %f b = %f r = %f d = %f po = %f\n", theta, a, b, bigr->r + smallr->r,
            gts_point_distance(GTS_POINT(bigr->centre), GTS_POINT(smallr->centre)),
            (bigr->r+smallr->r) / gts_point_distance(GTS_POINT(bigr->centre), GTS_POINT(smallr->centre)));

        printf("bigr centre = %f,%f smallr centre = %f,%f\n\n", vx(bigr->centre), vy(bigr->centre), 
            vx(smallr->centre), vy(smallr->centre));
#endif      
        g_assert(winddir);

        if(bigr==parc) winddir = -winddir;

        if(winddir == bigr->dir) {
          if(bigr==arc) {
          }else{
            gint wind1, wind2;

            wind1 = coord_wind(vx(bigr->centre), vy(bigr->centre), bigr->x0, bigr->y0, a0x, a0y);
            wind2 = coord_wind(vx(bigr->centre), vy(bigr->centre), a0x, a0y, vx(smallr->centre), vy(smallr->centre));

            if(wind1 != wind2) {
#ifdef DEBUG_EXPORT          
              printf("WARNING: BS TWIST DETECTED %f,%f %f,%f\n", vx(bigr->centre), vy(bigr->centre), vx(smallr->centre), vy(smallr->centre));
#endif
//              if(smallr->centre->flags & VERTEX_FLAG_FAKE) fliplist = g_slist_prepend(fliplist, smallr);
              remlist = g_slist_prepend(remlist, bigr);    
            }
          }
        }else{
          if(bigr==arc) {
          }else{
            gint wind1, wind2;

            wind1 = coord_wind(vx(bigr->centre), vy(bigr->centre), bigr->x0, bigr->y0, a1x, a1y);
            wind2 = coord_wind(vx(bigr->centre), vy(bigr->centre), a1x, a1y, vx(smallr->centre), vy(smallr->centre));

            if(wind1 != wind2) {
#ifdef DEBUG_EXPORT          
              printf("WARNING: BS TWIST DETECTED %f,%f %f,%f\n", vx(bigr->centre), vy(bigr->centre), vx(smallr->centre), vy(smallr->centre));
#endif
//              if(smallr->centre->flags & VERTEX_FLAG_FAKE) fliplist = g_slist_prepend(fliplist, smallr);
              remlist = g_slist_prepend(remlist, bigr);    
            }
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
            gint wind1, wind2;

            wind1 = coord_wind(vx(smallr->centre), vy(smallr->centre), smallr->x0, smallr->y0, a0x, a0y);
            wind2 = coord_wind(vx(smallr->centre), vy(smallr->centre), a0x, a0y, vx(bigr->centre), vy(bigr->centre));

            if(wind1 != wind2) {
#ifdef DEBUG_EXPORT          
              printf("WARNING: SB TWIST DETECTED %f,%f %f,%f\n", vx(bigr->centre), vy(bigr->centre), vx(smallr->centre), vy(smallr->centre));
#endif 
//              if(smallr->centre->flags & VERTEX_FLAG_FAKE) fliplist = g_slist_prepend(fliplist, smallr);
              remlist = g_slist_prepend(remlist, smallr);   
            }
          }else{
          }
        }else{
          if(bigr==arc) {
            gint wind1, wind2;

            wind1 = coord_wind(vx(smallr->centre), vy(smallr->centre), smallr->x0, smallr->y0, a1x, a1y);
            wind2 = coord_wind(vx(smallr->centre), vy(smallr->centre), a1x, a1y, vx(bigr->centre), vy(bigr->centre));

            if(wind1 != wind2) {
#ifdef DEBUG_EXPORT          
              printf("WARNING: SB TWIST DETECTED %f,%f %f,%f\n", vx(bigr->centre), vy(bigr->centre), vx(smallr->centre), vy(smallr->centre));
#endif
//              if(smallr->centre->flags & VERTEX_FLAG_FAKE) fliplist = g_slist_prepend(fliplist, smallr);
              remlist = g_slist_prepend(remlist, smallr);    
            }
          }else{
          }
        }

//*/

      }
    }

    parc = arc;

    i = i->next;
  }


  j = remlist;
  while(j) {
    toporouter_arc_t *arc = TOPOROUTER_ARC(j->data);

    printf("overshoot check deleting arc %f,%f\n", vx(arc->centre), vy(arc->centre));  
    toporouter_arc_remove(oproute, arc);

    j = j->next;
  }

  j = fliplist;
  while(j) {
    toporouter_arc_t *arc = TOPOROUTER_ARC(j->data);

    vertex_move_towards_vertex(GTS_VERTEX(arc->centre), GTS_VERTEX(arc->v), arc->r * 2.);
    arc->dir = -arc->dir;

    j = j->next;
  }

  g_slist_free(remlist);
  g_slist_free(fliplist);

}

void
fix_colinear_oproute_arcs(toporouter_oproute_t *oproute)
{
  GList *i = oproute->arcs;
  GSList *remlist = NULL, *j;
  toporouter_arc_t *parc = NULL, *arc = NULL;

  i = oproute->arcs;
  while(i) {
    toporouter_arc_t *narc = (toporouter_arc_t *)i->data;
    gint wind;

    if(oproute->serp && g_list_find(oproute->serp->arcs, narc)) {
      i = i->next;
      continue;
    }
  
    if(parc && arc && parc->dir == arc->dir && arc->dir == narc->dir) {
      wind = vertex_wind(GTS_VERTEX(parc->centre), GTS_VERTEX(arc->centre), GTS_VERTEX(narc->centre));
      if(!wind) {
        if(arc->r <= parc->r + EPSILON && arc->r <= narc->r + EPSILON) {
          remlist = g_slist_prepend(remlist, arc);
        }

      }

    }

    parc = arc;
    arc = narc;

    i = i->next;
  }


  j = remlist;
  while(j) {
    toporouter_arc_t *arc = TOPOROUTER_ARC(j->data);
    printf("colinear check deleting arc %f,%f\n", vx(arc->centre), vy(arc->centre));  
    toporouter_arc_remove(oproute, arc);

    j = j->next;
  }

  g_slist_free(remlist);

}


toporouter_arc_t *
vertex_child_arc(toporouter_vertex_t *v)
{
  if(v->arc) return v->arc;
  if(v->child) return vertex_child_arc(v->child);
  return NULL;
}

toporouter_arc_t *
vertex_parent_arc(toporouter_vertex_t *v)
{
  if(v->arc) return v->arc;
  if(v->parent) return vertex_parent_arc(v->parent);
  return NULL;
}


toporouter_vertex_t *
vertex_path_closest_to_vertex(toporouter_vertex_t *a, toporouter_vertex_t *b)
{
  toporouter_vertex_t *temp = a, *closest = NULL;
  gdouble d = 0.;

  while(temp && (temp->flags & VERTEX_FLAG_ROUTE)) {
    if(!closest) {
      closest = temp;
      d = gts_point_distance2(GTS_POINT(b), GTS_POINT(temp));
    }else{
      gdouble tempd = gts_point_distance2(GTS_POINT(b), GTS_POINT(temp));
      if(tempd < d) {
        closest = temp;
        d = tempd;
      }
    }
    temp = temp->parent;
  }

  temp = a;
  while(temp && (temp->flags & VERTEX_FLAG_ROUTE)) {
    if(!closest) {
      closest = temp;
      d = gts_point_distance2(GTS_POINT(b), GTS_POINT(temp));
    }else{
      gdouble tempd = gts_point_distance2(GTS_POINT(b), GTS_POINT(temp));
      if(tempd < d) {
        closest = temp;
        d = tempd;
      }
    }
    temp = temp->child;
  }

  g_assert(closest);
  return closest;
}


toporouter_arc_t *
check_line_clearance(toporouter_oproute_t *oproute, gdouble x0, gdouble y0, gdouble x1, gdouble y1, toporouter_vertex_t *centre, gdouble r, gdouble ms, toporouter_vertex_t *v)
{
  gdouble x, y;

  vertex_line_normal_intersection(x0, y0, x1, y1, vx(centre), vy(centre), &x, &y);

  if(x > MIN(x0,x1) && x < MAX(x0,x1) && y > MIN(y0,y1) && y < MAX(y0,y1)) {
    gdouble d = sqrt(pow(vx(centre) - x, 2) + pow(vy(centre) - y, 2));

    if(d - r  < ms - EPSILON) {
      toporouter_arc_t *rarc;
#ifdef DEBUG_EXPORT      
      printf("CLEARANCE VIOLATION between %f,%f and %f,%f ms = %f d = %f r = %f\n", x, y, 
          vx(centre), vy(centre),
          ms, d, r); 
      printf("p0 = %f,%f p1 = %f,%f\n", x0, y0, x1, y1);
#endif      
      rarc = toporouter_arc_new(oproute, v, centre, r + ms, -coord_wind(x0, y0, vx(centre), vy(centre), x1, y1));
      //rarc->vs = g_slist_prepend(rarc->vs, v);
      vertex_path_closest_to_vertex(v, centre)->arc = rarc;
      return rarc;
    }

  }

  return NULL;
}

guint
arc_on_opposite_side(toporouter_vertex_t *v, toporouter_arc_t *arc, toporouter_vertex_t *a)
{
  gint vwind, awind;

  g_assert(arc);
  g_assert(arc->centre);

  if(v->parent) {
    vwind = vertex_wind(GTS_VERTEX(v), GTS_VERTEX(v->parent), GTS_VERTEX(arc->centre));
    awind = vertex_wind(GTS_VERTEX(v), GTS_VERTEX(v->parent), GTS_VERTEX(a));
    if(awind == vwind) return 0;
  }
  
  if(v->child) {
    vwind = vertex_wind(GTS_VERTEX(v), GTS_VERTEX(v->child), GTS_VERTEX(arc->centre));
    awind = vertex_wind(GTS_VERTEX(v), GTS_VERTEX(v->child), GTS_VERTEX(a));
    if(awind == vwind) return 0;
  }

  return 1;
}


toporouter_arc_t *
check_oproute_edge(toporouter_oproute_t *oproute, gdouble x0, gdouble y0, gdouble x1, gdouble y1, GList *vlist, toporouter_vertex_t *v, toporouter_arc_t *arc,
    toporouter_arc_t *parc)
{
  toporouter_arc_t *childarc, *parentarc;
  toporouter_arc_t *rarc = NULL;

  if(vlist->next) {
    GList *curvlist = vlist->next;
    toporouter_vertex_t *nextv; 
    gint nextvwind;
    
    nextv = TOPOROUTER_VERTEX(curvlist->data);
    nextvwind = coord_wind(x0, y0, x1, y1, vx(nextv), vy(nextv));

    childarc = vertex_child_arc(nextv);
    parentarc = vertex_parent_arc(nextv);

#ifdef DEBUG_CHECK_OPROUTE          
    printf("NEXT HAS ");
    if(childarc) {
      g_assert(childarc->centre);
      printf("childarc (%f,%f) ", vx(childarc->centre), vy(childarc->centre));
    }else printf("no childarc ");

    if(parentarc) printf("parentarc (%f,%f) ", vx(parentarc->centre), vy(parentarc->centre));
    else printf("no parentarc ");
    
    if(childarc && arc_on_opposite_side(nextv, childarc, v)) printf("childarc on opposite side\n");
    if(parentarc && arc_on_opposite_side(nextv, parentarc, v)) printf("parentarc on opposite side\n");
    printf("\n");
#endif
    
    if(childarc && childarc != parc && childarc != arc && arc_on_opposite_side(nextv, childarc, v)) {
//    if(childarc) {
      gint childwind = coord_wind(x0, y0, x1, y1, vx(childarc->centre), vy(childarc->centre));
      if(childwind == nextvwind) {
        rarc = check_line_clearance(oproute, x0, y0, x1, y1, childarc->centre, childarc->r, min_spacing(v, nextv), v);
        if(rarc) {
#ifdef DEBUG_CHECK_OPROUTE          
          printf("case 0 %d %d\n", childwind, nextvwind);
#endif          
          return rarc;
        }
      }
    }
    if(parentarc && parentarc != childarc && parentarc != parc && parentarc != arc && arc_on_opposite_side(nextv, parentarc, v)) {
//    if(parentarc && parentarc != childarc) {
      gint parentwind = coord_wind(x0, y0, x1, y1, vx(parentarc->centre), vy(parentarc->centre));
      if(parentwind == nextvwind) {
        rarc = check_line_clearance(oproute, x0, y0, x1, y1, parentarc->centre, parentarc->r, min_spacing(v, nextv), v);
        if(rarc) {
#ifdef DEBUG_CHECK_OPROUTE          
          printf("case 1 %d %d\n", parentwind, nextvwind);
#endif        
          return rarc;
        }
      
      }
    }

  }else{
    guint check = 1;

    if(v->parent && v->parent->routingedge && 
        TOPOROUTER_IS_CONSTRAINT(v->parent->routingedge) && 
        segment_common_vertex(GTS_SEGMENT(v->parent->routingedge), GTS_SEGMENT(v->routingedge)))
      check = 0;

    if(v->child && v->child->routingedge && 
        TOPOROUTER_IS_CONSTRAINT(v->child->routingedge) && 
        segment_common_vertex(GTS_SEGMENT(v->child->routingedge), GTS_SEGMENT(v->routingedge)))
      check = 0;
    
    if(vertex_netlist(tedge_v2(v->routingedge)) && vertex_netlist(tedge_v2(v->routingedge)) == oproute->netlist)
      check = 0;

    if(check) {
      rarc = check_line_clearance(oproute, x0, y0, x1, y1, tedge_v2(v->routingedge), 0., min_spacing(v, tedge_v2(v->routingedge)), v);
      if(rarc) {
#ifdef DEBUG_CHECK_OPROUTE          
        printf("netlist of tedge_v2 = %s\n", vertex_netlist(tedge_v2(v->routingedge)));
#endif        
        return rarc;
      }
    }else{
#ifdef DEBUG_CHECK_OPROUTE          
      printf(" not checking tedge_v2 %f,%f\n", vx(tedge_v2(v->routingedge)), vy(tedge_v2(v->routingedge)) );
#endif        
    }
  }

  if(vlist->prev) {
    toporouter_vertex_t *prevv; 
    gint prevvwind;
    GList *curvlist = vlist->prev;
    
    prevv = TOPOROUTER_VERTEX(curvlist->data);
    prevvwind = coord_wind(x0, y0, x1, y1, vx(prevv), vy(prevv));

    childarc = vertex_child_arc(prevv);
    parentarc = vertex_parent_arc(prevv);
    
#ifdef DEBUG_CHECK_OPROUTE          
    printf("PREV HAS ");
    if(childarc) printf("childarc (%f,%f) ", vx(childarc->centre), vy(childarc->centre));
    else printf("no childarc ");

    if(parentarc) printf("parentarc (%f,%f) ", vx(parentarc->centre), vy(parentarc->centre));
    else printf("no parentarc ");

    if(childarc && arc_on_opposite_side(prevv, childarc, v)) printf("childarc on opposite side\n");
    if(parentarc && arc_on_opposite_side(prevv, parentarc, v)) printf("parentarc on opposite side\n");
    printf("\n");
#endif

//    if(childarc) {
    if(childarc && childarc != parc && childarc != arc && arc_on_opposite_side(prevv, childarc, v)) {
      gint childwind = coord_wind(x0, y0, x1, y1, vx(childarc->centre), vy(childarc->centre));
      if(childwind == prevvwind) {
        rarc = check_line_clearance(oproute, x0, y0, x1, y1, childarc->centre, childarc->r, min_spacing(v, prevv), v);
        if(rarc) {
#ifdef DEBUG_CHECK_OPROUTE          
          printf("case 2 %d %d\n", childwind, prevvwind);
#endif        
          return rarc;
        }
      }
    }
    if(parentarc && parentarc != childarc && parentarc != parc && parentarc != arc && arc_on_opposite_side(prevv, parentarc, v)) {
      gint parentwind = coord_wind(x0, y0, x1, y1, vx(parentarc->centre), vy(parentarc->centre));
      if(parentwind == prevvwind) {
        rarc = check_line_clearance(oproute, x0, y0, x1, y1, parentarc->centre, parentarc->r, min_spacing(v, prevv), v);
        if(rarc) {
#ifdef DEBUG_CHECK_OPROUTE          
          printf("case 3 %d %d\n", parentwind, prevvwind);
#endif        
          return rarc;
        }

      }
    }


  }else{
    guint check = 1;

    if(v->parent && v->parent->routingedge && 
        TOPOROUTER_IS_CONSTRAINT(v->parent->routingedge) && 
        segment_common_vertex(GTS_SEGMENT(v->parent->routingedge), GTS_SEGMENT(v->routingedge)))
      check = 0;

    if(v->child && v->child->routingedge && 
        TOPOROUTER_IS_CONSTRAINT(v->child->routingedge) && 
        segment_common_vertex(GTS_SEGMENT(v->child->routingedge), GTS_SEGMENT(v->routingedge)))
      check = 0;

    if(vertex_netlist(tedge_v1(v->routingedge)) && vertex_netlist(tedge_v1(v->routingedge)) == oproute->netlist)
      check = 0;

    if(check) {
      rarc = check_line_clearance(oproute, x0, y0, x1, y1, tedge_v1(v->routingedge), 0., min_spacing(v, tedge_v1(v->routingedge)), v);
      if(rarc) {
#ifdef DEBUG_CHECK_OPROUTE          
        printf("netlist of tedge_v1 = %s\n", vertex_netlist(tedge_v1(v->routingedge)));
#endif        
        return rarc;
      }
    }else{
#ifdef DEBUG_CHECK_OPROUTE          
      printf(" not checking tedge_v1 %f,%f\n", vx(tedge_v1(v->routingedge)), vy(tedge_v1(v->routingedge)) );
#endif
    }
  }


  return NULL;
}

toporouter_arc_t *
oproute_contains_arc(toporouter_oproute_t *oproute, toporouter_vertex_t *centre)
{
  GList *i = oproute->arcs;
  while(i) {
    toporouter_arc_t *arc = TOPOROUTER_ARC(i->data);
    if(arc->centre == centre) return arc;

    i = i->next;
  }

  return NULL;
}

toporouter_arc_t *
check_oproute(toporouter_t *r, toporouter_oproute_t *oproute) 
{
  toporouter_arc_t *parc = NULL, *arc = NULL, *rarc;
  toporouter_vertex_t *v;//, *ev;

  GList *i = oproute->arcs;

//  printf("checking oproute for %s\n", oproute->netlist);

  while(i) {
    arc = (toporouter_arc_t *)i->data;
    
//    ev = TOPOROUTER_VERTEX(arc->vs->data);

    if(parc && arc) {
//      v = TOPOROUTER_VERTEX(g_slist_last(parc->vs)->data)->child;
//      g_assert(parc->vs);      
//      v = TOPOROUTER_VERTEX(parc->vs->data)->child;
      v = parc->v;

#ifdef DEBUG_CHECK_OPROUTE          
      printf("arc = %f,%f parc = %f,%f\n", vx(arc->centre), vy(arc->centre),
          vx(parc->centre), vy(parc->centre));
      printf("arcv = %f,%f parcv = %f,%f\n", vx(arc->v), vy(arc->v),
          vx(parc->v), vy(parc->v));
#endif
//      while(v && v != ev) {
      while(v && v->flags & VERTEX_FLAG_ROUTE) {
#ifdef DEBUG_CHECK_OPROUTE          
        printf("arc-parc v = %f,%f\n\n", vx(v), vy(v));
#endif

        if(v->routingedge) {
          GList *vlist = g_list_find(edge_routing(v->routingedge), v);

          rarc = check_oproute_edge(oproute, parc->x1, parc->y1, arc->x0, arc->y0, vlist, v, arc, parc);

          if(rarc) {
            toporouter_arc_t *dupearc;
            if((dupearc = oproute_contains_arc(oproute, rarc->centre))) {
              toporouter_vertex_t *tempv = rarc->v;
              dupearc->r = rarc->r;
              toporouter_arc_remove(oproute, rarc);
              tempv->arc = dupearc;
              rarc = dupearc;
            }else{
              oproute->arcs = g_list_insert(oproute->arcs, rarc, g_list_index(oproute->arcs, arc));
            }
            return rarc;
          }
        }

        if(v->arc && v->arc != parc) break;

        v = v->child;
      }

    }else{

      v = oproute->term1;

      while(v && v != oproute->term2) {
#ifdef DEBUG_CHECK_OPROUTE          
        printf("term1-arc v = %f,%f\n\n", vx(v), vy(v));
#endif
        if(v->routingedge) {
          GList *vlist = g_list_find(edge_routing(v->routingedge), v);
          rarc = check_oproute_edge(oproute, vx(oproute->term1), vy(oproute->term1), arc->x0, arc->y0, vlist, v, arc, parc);

          if(rarc) {
            toporouter_arc_t *dupearc;

            if((dupearc = oproute_contains_arc(oproute, rarc->centre))) {
              toporouter_vertex_t *tempv = rarc->v;
              dupearc->r = rarc->r;
              toporouter_arc_remove(oproute, rarc);
              tempv->arc = dupearc;
              rarc = dupearc;
            }else{
//              oproute->arcs = g_list_insert(oproute->arcs, rarc, g_list_index(oproute->arcs, arc));
              oproute->arcs = g_list_prepend(oproute->arcs, rarc);
            }
            return rarc;
          }

        }
        if(v->arc) break;
        
        if(v == oproute->term1) 
          v = TOPOROUTER_VERTEX(oproute->path->next->data);
        else
          v = v->child;
      }


    }

    parc = arc;
    i = i->next;
  }

  if(parc) {
    //ev = oproute->term2;
    //v = TOPOROUTER_VERTEX(parc->vs->data)->child;
    v = parc->v;

    while(v) {
#ifdef DEBUG_CHECK_OPROUTE          
      printf("parc-term2 v = %f,%f\n\n", vx(v), vy(v));
#endif

      if(v->routingedge) {
        GList *vlist = g_list_find(edge_routing(v->routingedge), v);

        rarc = check_oproute_edge(oproute, parc->x1, parc->y1, vx(oproute->term2), vy(oproute->term2), vlist, v, arc, parc);
        if(rarc) {
          toporouter_arc_t *dupearc;
          if((dupearc = oproute_contains_arc(oproute, rarc->centre))) {
            toporouter_vertex_t *tempv = rarc->v;
            dupearc->r = rarc->r;
            toporouter_arc_remove(oproute, rarc);
            tempv->arc = dupearc;
            rarc = dupearc;
          }else{
//            oproute->arcs = g_list_insert(oproute->arcs, rarc, g_list_index(oproute->arcs, arc));
            oproute->arcs = g_list_append(oproute->arcs, rarc);
          }
          return rarc;
        }
      }
      if(!(v->flags & VERTEX_FLAG_ROUTE)) break;
      v = v->child;
    }

  }
  return NULL;
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

gint  
oproute_tof_compare(toporouter_oproute_t **a, toporouter_oproute_t **b)
{
  if((*a)->tof < (*b)->tof) return -1;
  if((*a)->tof > (*b)->tof) return 1;
  return 0;
}

void
print_oproute_tof(toporouter_oproute_t *oproute, gdouble critical)
{
  printf("%15f\t%15f\t%15d\t%15s\n", oproute->tof, critical - oproute->tof, g_list_length(oproute->arcs), oproute->netlist);
}

toporouter_vertex_t *
toporouter_arc_or_vertex_next_vertex(gpointer data)
{
  toporouter_vertex_t *v;
  
  if(TOPOROUTER_IS_ARC(data)) {
    v = TOPOROUTER_ARC(data)->v->child;
  }else{
    g_assert(TOPOROUTER_IS_VERTEX(data));
    v = TOPOROUTER_VERTEX(data)->child;
  }

  while(v) {
    if(!(v->flags & VERTEX_FLAG_ROUTE) || v->arc) break;
    v = v->child;
  }

  return v;
}

GList *
toporouter_vertex_get_adj_non_route_vertices(toporouter_vertex_t *v)
{
  GList *rval = NULL;
  toporouter_vertex_t *tempv = v->child;

  if(!(v->flags & VERTEX_FLAG_ROUTE)) return g_list_prepend(NULL, v);

  while(tempv) {
    if(tempv->arc) {
      rval = g_list_prepend(rval, tempv->arc);
      break;
    }
    if(!(tempv->flags & VERTEX_FLAG_ROUTE)) {
      rval = g_list_prepend(rval, tempv);
      break;
    }
    tempv = tempv->child;
  }

  tempv = tempv->parent;
  while(tempv) {
    if(tempv->arc) {
      rval = g_list_prepend(rval, tempv->arc);
      break;
    }
    if(!(tempv->flags & VERTEX_FLAG_ROUTE)) {
      rval = g_list_prepend(rval, tempv);
      break;
    }
    tempv = tempv->parent;
  }

  return rval;
}



guint
path_adjacent_to_vertex(GSList *path, toporouter_vertex_t *v)
{
  while(path) {
    toporouter_vertex_t *curv = TOPOROUTER_VERTEX(path->data);
    GList *list;
    
    if(curv->flags & VERTEX_FLAG_ROUTE) {
      list = g_list_find(edge_routing(curv->routingedge), curv);
      if(!list->next && tedge_v2(curv->routingedge) == v)
        return 1;
      if(!list->prev && tedge_v1(curv->routingedge) == v)
        return 1;
    }

    path = path->next;
  }

  return 0;
}

toporouter_clearance_t *
toporouter_clearance_new(gpointer data, gdouble x0, gdouble y0, gdouble x1, gdouble y1, gint *wind)
{
  toporouter_clearance_t *clearance = malloc(sizeof(toporouter_clearance_t));
  clearance->data = data;
 
  if(wind) {
    clearance->wind = *wind;
  }else{
    if(TOPOROUTER_IS_ARC(data)) {
      toporouter_arc_t *arc = TOPOROUTER_ARC(data);
      clearance->wind = coord_wind(x0, y0, x1, y1, vx(arc->centre), vy(arc->centre));

    }else{
      toporouter_vertex_t *v = TOPOROUTER_VERTEX(data);
      g_assert(TOPOROUTER_IS_VERTEX(data));
      clearance->wind = coord_wind(x0, y0, x1, y1, vx(v), vy(v));
    }
  }
  return clearance;
}

void
toporouter_segment_calculate_clearances(toporouter_oproute_t *oproute, GList **clearance, gdouble x0, gdouble y0, gdouble x1, gdouble y1,
    toporouter_vertex_t *v)
{
  GSList *oproutes = oproute->adj;
  gdouble linedata[4] = {x0, y0, x1, y1};
  gdouble line_int_x, line_int_y;

  while(oproutes) {
    toporouter_oproute_t *adj = TOPOROUTER_OPROUTE(oproutes->data);
    toporouter_vertex_t *pv = adj->term1;

    GList *adjarcs = adj->arcs;
    while(adjarcs) {
      toporouter_arc_t *adjarc = TOPOROUTER_ARC(adjarcs->data);
      //toporouter_vertex_t *nv = adjarcs->next ? TOPOROUTER_ARC(adjarcs->next->data)->centre : adj->term2;

      if(!path_adjacent_to_vertex(oproute->path, adjarc->centre))
      if(vertex_line_normal_intersection(x0, y0, x1, y1, vx(adjarc->centre), vy(adjarc->centre), &line_int_x, &line_int_y)) {
//        if(vertex_wind(GTS_VERTEX(pv), GTS_VERTEX(nv), GTS_VERTEX(adjarc->centre)) == coord_wind(vx(pv), vy(pv), vx(nv), vy(nv), line_int_x, line_int_y)) {
          if(!g_list_find(*clearance, adjarc))
            *clearance = g_list_insert_sorted_with_data(*clearance, toporouter_clearance_new(adjarc, x0, y0, x1, y1, NULL), clearance_list_compare, &linedata);

//        }
      }

      pv = adjarc->centre;

      adjarcs = adjarcs->next;
    }

    oproutes = oproutes->next;
  }

  while(v && !v->arc && v->flags & VERTEX_FLAG_ROUTE) {
    GList *list = g_list_find(edge_routing(v->routingedge), v);

    if(!list->next && !g_list_find(*clearance, tedge_v2(v->routingedge))) {
      toporouter_vertex_t *tv = tedge_v2(v->routingedge);
      if(vertex_line_normal_intersection(x0, y0, x1, y1, vx(tv), vy(tv), &line_int_x, &line_int_y)) {
        if(!g_list_find(*clearance, tv))
          *clearance = g_list_insert_sorted_with_data(*clearance, toporouter_clearance_new(tv, x0, y0, x1, y1, NULL), clearance_list_compare, &linedata);
      }
    }

    if(!list->prev && !g_list_find(*clearance, tedge_v1(v->routingedge))) {
      toporouter_vertex_t *tv = tedge_v1(v->routingedge);
      if(vertex_line_normal_intersection(x0, y0, x1, y1, vx(tv), vy(tv), &line_int_x, &line_int_y)) {
        if(!g_list_find(*clearance, tv))
          *clearance = g_list_insert_sorted_with_data(*clearance, toporouter_clearance_new(tv, x0, y0, x1, y1, NULL), clearance_list_compare, &linedata);
      }
    }

    v = v->child;
  }

}

void
toporouter_oproute_calculate_clearances(toporouter_oproute_t *oproute) 
{
  GList *i = oproute->arcs, **pclearance;
  gdouble px = vx(oproute->term1), py = vy(oproute->term1);
  toporouter_vertex_t *pv = TOPOROUTER_VERTEX(oproute->path->next->data);

  if(oproute->clearance) {
    g_list_free(oproute->clearance);
    oproute->clearance = NULL;
  }

  pclearance = &(oproute->clearance);
  
  while(i) {
    toporouter_arc_t *arc = TOPOROUTER_ARC(i->data);
    if(arc->clearance) {
      g_list_free(arc->clearance);
      arc->clearance = NULL;
    }
    toporouter_segment_calculate_clearances(oproute, pclearance, px, py, arc->x0, arc->y0, pv);

    pclearance = &(arc->clearance);
    px = arc->x1;
    py = arc->y1;
    if(arc->v)
      pv = arc->v->child;

    i = i->next;
  }
  
  toporouter_segment_calculate_clearances(oproute, pclearance, px, py, vx(oproute->term2), vy(oproute->term2), pv);

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

void
oproute_print_clearances(toporouter_oproute_t *oproute) 
{
  GList *i = oproute->arcs;
  gdouble x0, y0, x1, y1;

  printf("\nterm1:\n"); print_vertex(oproute->term1);
  x0 = vx(oproute->term1);
  y0 = vy(oproute->term1);
  if(i) {
    toporouter_arc_t *narc = TOPOROUTER_ARC(i->data);
    x1 = narc->x0;
    y1 = narc->y0;
  }else{
    x1 = vx(oproute->term2);
    y1 = vy(oproute->term2);
  }
  
  print_clearance_list(oproute->clearance, x0, y0, x1, y1);

  while(i) {
    toporouter_arc_t *arc = TOPOROUTER_ARC(i->data);

    x0 = arc->x1;
    y0 = arc->y1;
    if(i->next) {
      toporouter_arc_t *narc = TOPOROUTER_ARC(i->next->data);
      x1 = narc->x0;
      y1 = narc->y0;
    }else{
      x1 = vx(oproute->term2);
      y1 = vy(oproute->term2);
    }
    
    printf("\narcv:\n"); print_vertex(arc->centre);
    print_clearance_list(arc->clearance, x0, y0, x1, y1);

    i = i->next;
  }

}

void
clearance_list_find_max_region(GList *clearance, gdouble x0, gdouble y0, gdouble x1, gdouble y1, 
    gdouble *max, gdouble *rx0, gdouble *ry0, gdouble *rx1, gdouble *ry1, guint debug)
{
  gdouble px = x0, py = y0;
  gdouble line_int_x, line_int_y, tempd;

  if(debug) {
    printf("\tfind_max_region: checking line %f,%f %f,%f\n", x0, y0, x1, y1);
  }

  while(clearance) {
    toporouter_vertex_t *v;
    toporouter_clearance_t *c = TOPOROUTER_CLEARANCE(clearance->data);

    if(TOPOROUTER_IS_ARC(c->data)) {
      v = TOPOROUTER_ARC(c->data)->centre;
    }else{
      g_assert(TOPOROUTER_IS_VERTEX(c->data));
      v = TOPOROUTER_VERTEX(c->data);
    }

    if(debug) {
      printf("\t\tfind_max_region: checking v %f,%f\n", vx(v), vy(v));
    }

    if(vertex_line_normal_intersection(x0, y0, x1, y1, vx(v), vy(v), &line_int_x, &line_int_y)) {
      tempd = sqrt(pow(line_int_x-px,2)+pow(line_int_y-py,2));
      if(!(*max) || tempd > *max) {
        *rx0 = px; *ry0 = py;
        *rx1 = line_int_x; *ry1 = line_int_y;
        *max = tempd;
        
      }
      px = line_int_x;
      py = line_int_y;
    }
    clearance = clearance->next;
  }

  tempd = sqrt(pow(x1-px,2)+pow(y1-py,2));
  if(!(*max) || tempd > *max) {
    *rx0 = px; *ry0 = py;
    *rx1 = x1; *ry1 = y1;
    *max = tempd;
  }

}

void
find_serpintine_start(toporouter_oproute_t *oproute, gdouble length_required, 
    gdouble *x, gdouble *y, guint *arcn,
    gdouble *rx0, gdouble *ry0, gdouble *rx1, gdouble *ry1, guint debug)
{
  GList *i = oproute->arcs;
  gdouble x0, y0, x1, y1;
//  gdouble rx0, ry0, rx1, ry1;
  gdouble max = 0., pmax = 0.;
  guint curarc = 0;

  *arcn = 0;

  x0 = vx(oproute->term1);
  y0 = vy(oproute->term1);
  if(i) {
    toporouter_arc_t *narc = TOPOROUTER_ARC(i->data);
    x1 = narc->x0;
    y1 = narc->y0;
  }else{
    x1 = vx(oproute->term2);
    y1 = vy(oproute->term2);
  }
  
  clearance_list_find_max_region(oproute->clearance, x0, y0, x1, y1, &max, rx0, ry0, rx1, ry1, debug);
  pmax = max;

  while(i) {
    toporouter_arc_t *arc = TOPOROUTER_ARC(i->data);
    
    curarc++;

    x0 = arc->x1;
    y0 = arc->y1;
    if(i->next) {
      toporouter_arc_t *narc = TOPOROUTER_ARC(i->next->data);
      x1 = narc->x0;
      y1 = narc->y0;
    }else{
      x1 = vx(oproute->term2);
      y1 = vy(oproute->term2);
    }
    
    clearance_list_find_max_region(arc->clearance, x0, y0, x1, y1, &max, rx0, ry0, rx1, ry1, debug);
    if(max > pmax) {
      *arcn = curarc;
      pmax = max;
    }

    i = i->next;
  }

  *x = (*rx0 + *rx1) / 2.;
  *y = (*ry0 + *ry1) / 2.;
}

void
oproute_foreach_adjseg(toporouter_t *r, toporouter_oproute_t *oproute, oproute_adjseg_func func, GSList **ignore)
{
  GSList *adjs = oproute->adj;

  while(adjs) {
    toporouter_oproute_t *adj = TOPOROUTER_OPROUTE(adjs->data);
    gdouble px = vx(adj->term1), py = vy(adj->term1);
    GList *arcs = adj->arcs;
    GList **pclearance = &(adj->clearance);
    guint arcn = 0;

    if(ignore && g_slist_find(*ignore, adj)) {
      adjs = adjs->next;
      continue;
    }

    while(arcs) {
      toporouter_arc_t *arc = TOPOROUTER_ARC(arcs->data);

      func(r, ignore, pclearance, &arcn, px, py, arc->x0, arc->y0, oproute, adj);

      px = arc->x1;
      py = arc->y1;
      pclearance = &(arc->clearance);

      arcn++;
      arcs = arcs->next;
    }

    func(r, ignore, pclearance, &arcn, px, py, vx(adj->term2), vy(adj->term2), oproute, adj);

    adjs = adjs->next;
  }

}

void
adjseg_serpintine_update(toporouter_t *r, GSList **ignore, GList **clearance, guint *arcn, gdouble x0, gdouble y0, gdouble x1, gdouble y1, toporouter_oproute_t *oproute,
    toporouter_oproute_t *adj)
{
  gdouble line_int_x, line_int_y;
  gdouble linedata[4] = { x0, y0, x1, y1 };
  guint counter = 0;
  gint wind = coord_wind(oproute->serp->x0, oproute->serp->y0, oproute->serp->x1, oproute->serp->y1, (x0+x1)/2., (y0+y1)/2.);
  gint inswind = coord_wind(x0, y0, x1, y1, oproute->serp->x, oproute->serp->y);

  GList *arcs = oproute->serp->arcs;
  while(arcs) {
    toporouter_arc_t *arc = TOPOROUTER_ARC(arcs->data);
    if(vertex_line_normal_intersection(x0, y0, x1, y1, vx(arc->centre), vy(arc->centre), &line_int_x, &line_int_y)) {
      gint serpwind = coord_wind(oproute->serp->x0, oproute->serp->y0, oproute->serp->x1, oproute->serp->y1, vx(arc->centre), vy(arc->centre));

      if(arcs->next && arcs->prev && serpwind == wind)
        *clearance = g_list_insert_sorted_with_data(*clearance, toporouter_clearance_new(arc, x0, y0, x1, y1, &inswind), clearance_list_compare, &linedata);
      else if((!arcs->next || !arcs->prev) && serpwind == -wind) 
        *clearance = g_list_insert_sorted_with_data(*clearance, toporouter_clearance_new(arc, x0, y0, x1, y1, &inswind), clearance_list_compare, &linedata);
      counter++;
    }

    arcs = arcs->next;
  }

//  printf("oproute %s updated %d serp arcs on %s\n", oproute->netlist, counter, adj->netlist);
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

void
serp_push(toporouter_t *tr, toporouter_serpintine_t *serp, toporouter_arc_t *worstarc, gdouble x0, gdouble y0, gdouble x1, gdouble y1)
{
  gint worstindex = g_list_index(serp->arcs, worstarc);
  toporouter_arc_t *pivot;
  gdouble pivot_line_int_x, pivot_line_int_y;
  gdouble ms, r, d, theta;
  gdouble x_theta0, y_theta0, x_theta1, y_theta1, x, y, basex, basey;
  gdouble delta_theta;
  GList *i = serp->arcs;

  printf("serp push: length = %d worstindex = %d\n", g_list_length(serp->arcs), worstindex);

  if(g_list_length(serp->arcs) - worstindex < worstindex) {
    pivot = TOPOROUTER_ARC(g_list_first(serp->arcs)->data);
  }else{
    pivot = TOPOROUTER_ARC(g_list_last(serp->arcs)->data);
  }

  vertex_line_normal_intersection(x0, y0, x1, y1, vx(pivot->centre), vy(pivot->centre), &pivot_line_int_x, &pivot_line_int_y);
  ms = oproute_min_spacing(pivot->oproute, worstarc->oproute) + worstarc->r;
  r = sqrt(pow(vx(pivot->centre)-vx(worstarc->centre),2)+pow(vy(pivot->centre)-vy(worstarc->centre),2));
  d = sqrt(pow(pivot_line_int_x-vx(pivot->centre),2)+pow(pivot_line_int_y-vy(pivot->centre),2));
  
  coord_move_towards_coord_values(vx(pivot->centre), vy(pivot->centre), pivot_line_int_x, pivot_line_int_y, r, &basex, &basey);
  basex -= vx(pivot->centre);
  basey -= vy(pivot->centre);

  theta = acos((ms-d)/-r);

  x_theta0 = (basex * cos(theta) - basey * sin(theta)) + vx(pivot->centre);
  y_theta0 = (basex * sin(theta) + basey * cos(theta)) + vy(pivot->centre);
  x_theta1 = (basex * cos(2.*M_PI-theta) - basey * sin(2.*M_PI-theta)) + vx(pivot->centre);
  y_theta1 = (basex * sin(2.*M_PI-theta) + basey * cos(2.*M_PI-theta)) + vy(pivot->centre);

  if(pow(x_theta0-vx(worstarc->centre),2)+pow(y_theta0-vy(worstarc->centre),2) < pow(x_theta1-vx(worstarc->centre),2)+pow(y_theta1-vy(worstarc->centre),2)){
    x = x_theta0; y = y_theta0;
  }else{
    x = x_theta1; y = y_theta1;
    theta = 2. * M_PI - theta;
  }

  delta_theta = -vector_angle(vx(pivot->centre), vy(pivot->centre), vx(worstarc->centre), vy(worstarc->centre), x, y);

  printf("pivot @ %f,%f ms = %f r = %f d = %f theta = %f\n", vx(pivot->centre), vy(pivot->centre), ms, r, d, theta);

  printf("new coord = %f,%f delta theta = %f\n", x, y, delta_theta);

  basex = vx(worstarc->centre) - vx(pivot->centre);
  basey = vy(worstarc->centre) - vy(pivot->centre);

  x = (basex * cos(delta_theta) - basey * sin(delta_theta)) + vx(pivot->centre);
  y = (basex * sin(delta_theta) + basey * cos(delta_theta)) + vy(pivot->centre);
  
  printf("new coord (after matrix transform) = %f,%f delta theta = %f\n", x, y, delta_theta);
  
  while(i) {
    toporouter_arc_t *arc = TOPOROUTER_ARC(i->data);

    if(arc == pivot) {
      i = i->next;
      continue;
    }

    basex = vx(arc->centre) - vx(pivot->centre);
    basey = vy(arc->centre) - vy(pivot->centre);

    x = (basex * cos(delta_theta) - basey * sin(delta_theta)) + vx(pivot->centre);
    y = (basex * sin(delta_theta) + basey * cos(delta_theta)) + vy(pivot->centre);

    GTS_POINT(arc->centre)->x = x;
    GTS_POINT(arc->centre)->y = y;
    
    i = i->next;
  }

  calculate_oproute(tr, worstarc->oproute);
}

void
arc_import_clearance(toporouter_arc_t *arc, GList **clearancelist, gdouble x0, gdouble y0, gdouble x1, gdouble y1)
{
  GList *i = clearancelist ? *clearancelist : NULL;
  
  printf("\t\tarc_import_clearance");
  
  while(i) {
    toporouter_clearance_t *c = TOPOROUTER_CLEARANCE(i->data);
    toporouter_vertex_t *v;
    gdouble line_int_x, line_int_y;

    if(TOPOROUTER_IS_ARC(c->data)) {
      v = TOPOROUTER_ARC(c->data)->centre;
    }else{
      g_assert(TOPOROUTER_IS_VERTEX(c->data));
      v = TOPOROUTER_VERTEX(c->data);
    }

    if(vertex_line_normal_intersection(x0, y0, x1, y1, vx(v), vy(v), &line_int_x, &line_int_y)) {
      gdouble linedata[4] = {x0, y0, x1, y1};
      arc->clearance = g_list_insert_sorted_with_data(arc->clearance, i->data, clearance_list_compare, &linedata);
      printf("*");
    }else
      printf(".");

    i = i->next;
  }
  
  printf("\n\t\t%d items in newarc clearancelist\n", g_list_length(arc->clearance));
/*

  i = arc->clearance;
  while(i) {
    *clearancelist = g_list_remove(*clearancelist, i->data);

    i = i->next;
  }
*/
}

toporouter_arc_t *
oproute_insert_arc(toporouter_oproute_t *oproute, guint arcn, toporouter_arc_t *arc, gdouble x0, gdouble y0, gdouble x1, gdouble y1,
    gdouble line_int_x, gdouble line_int_y, gdouble ms)
{
  gdouble properx, propery;
  gint dir;
  toporouter_arc_t *newarc;
  GList *i = oproute->arcs;

  while(i) {
    toporouter_arc_t *curarc = TOPOROUTER_ARC(i->data);
    if(curarc->centre == arc->centre) {
      printf("WARNING: tried to insert a dupe arc, ignoring\n");
      return NULL;
    }
    i = i->next;
  }


  g_assert(vx(arc->centre) != line_int_x && vy(arc->centre) != line_int_y);
  coord_move_towards_coord_values(vx(arc->centre), vy(arc->centre), line_int_x, line_int_y, ms, &properx, &propery);
  dir = coord_wind(x0, y0, properx, propery, x1, y1);

  newarc = toporouter_arc_new(oproute, NULL, arc->centre, ms, dir);
  
  oproute->arcs = g_list_insert(oproute->arcs, newarc, arcn);
  printf("\tinserting arc centre %f,%f radius %f\n", vx(arc->centre), vy(arc->centre), ms);
  return newarc;
}

void
adjseg_check_clearance(toporouter_t *r, GSList **ignore, GList **clearancelist, guint *arcn, gdouble x0, gdouble y0, gdouble x1, gdouble y1, toporouter_oproute_t *oproute,
    toporouter_oproute_t *adj)
{
  GList *arcs = oproute->arcs;
  guint recalculate = 0;

  printf("checking lines of %s against arcs of %s\n", adj->netlist, oproute->netlist);

  while(arcs) {
    toporouter_arc_t *arc = TOPOROUTER_ARC(arcs->data);
    gdouble line_int_x, line_int_y;
    
    if(!path_adjacent_to_vertex(adj->path, arc->centre))
      if(vertex_line_normal_intersection(x0, y0, x1, y1, vx(arc->centre), vy(arc->centre), &line_int_x, &line_int_y)) {
        gdouble ms = oproute_min_spacing(adj, oproute) + arc->r;
        gdouble d = sqrt(pow(line_int_x-vx(arc->centre),2)+pow(line_int_y-vy(arc->centre),2));

        if(d < ms) {
          toporouter_arc_t *newarc;
          printf("\tfailure with arc->centre %f,%f and line %f,%f %f,%f\n", vx(arc->centre), vy(arc->centre), x0, y0, x1, y1);

          newarc = oproute_insert_arc(adj, (*arcn)++, arc, x0, y0, x1, y1, line_int_x, line_int_y, ms);
          if(newarc) arc_import_clearance(newarc, clearancelist, vx(newarc->centre), vy(newarc->centre), x1, y1);

          recalculate = 1;
        }

      }

    arcs = arcs->next;
  }

  if(recalculate) {
    GSList *tempignore = ignore ? g_slist_copy(*ignore) : NULL;

    fix_colinear_oproute_arcs(adj);
    calculate_oproute(r, adj);
    fix_overshoot_oproute_arcs(adj);
    calculate_oproute(r, adj);

    tempignore = g_slist_prepend(tempignore, oproute);
    oproute_foreach_adjseg(r, adj, adjseg_check_clearance, &tempignore);
    g_slist_free(tempignore);
  }

}


void
adjseg_check_serp_clearances(toporouter_t *r, GSList **ignore, GList **clearancelist, guint *arcn, gdouble x0, gdouble y0, gdouble x1, gdouble y1, toporouter_oproute_t *oproute,
    toporouter_oproute_t *adj)
{
  GList *clearances = *clearancelist;
  gdouble line_int_x, line_int_y;
  toporouter_arc_t *worstarc = NULL;
  gdouble worstd = 0.;

//  printf("check serp clearances for %s\n", adj->netlist);

  while(clearances) {
    toporouter_clearance_t *clearance = TOPOROUTER_CLEARANCE(clearances->data);
    
    if(TOPOROUTER_IS_ARC(clearance->data)) {
      toporouter_arc_t *arc = TOPOROUTER_ARC(clearance->data);
      
      gint wind = coord_wind(x0, y0, x1, y1, vx(arc->centre), vy(arc->centre));

//      printf("\tarc @ %f,%f line %f,%f %f,%f", vx(arc->centre), vy(arc->centre), x0, y0, x1, y1);

      if(wind != clearance->wind) {
        // arc is on wrong side of line
//        printf("wrong side ");
        
        if(vertex_line_normal_intersection(x0, y0, x1, y1, vx(arc->centre), vy(arc->centre), &line_int_x, &line_int_y)) {
          gdouble d = -sqrt(pow(line_int_x-vx(arc->centre),2)+pow(line_int_y-vy(arc->centre),2));

          if((!worstarc) || (d < worstd)) {
            worstarc = arc;
            worstd = d;
          }
//          printf("line int\n");
        }
//        else printf("no line int\n");
      }else{
//        printf("right side ");

        if(vertex_line_normal_intersection(x0, y0, x1, y1, vx(arc->centre), vy(arc->centre), &line_int_x, &line_int_y)) {
          gdouble d = sqrt(pow(line_int_x-vx(arc->centre),2)+pow(line_int_y-vy(arc->centre),2));
          gdouble ms = oproute_min_spacing(adj, oproute) + arc->r - EPSILON;

          if((!worstarc && d < ms) || (d < ms && d < worstd)) {
            worstarc = arc;
            worstd = d;
          }
//          printf("line int\n");
        }
//        else printf("no line int\n");
      }

    }

    clearances = clearances->next;
  }

  if(worstarc) {
    GSList *tempignore = ignore ? g_slist_copy(*ignore) : NULL;

    printf("SERP ARC CLEARANCE FAIL: d = %f arc->centre = %f,%f line = %f,%f %f,%f\n",
        worstd, vx(worstarc->centre), vy(worstarc->centre), x0, y0, x1, y1);
    g_assert(worstarc->oproute->serp);

    serp_push(r, worstarc->oproute->serp, worstarc, x0, y0, x1, y1);

    tempignore = g_slist_prepend(tempignore, oproute);
    oproute_foreach_adjseg(r, worstarc->oproute, adjseg_check_clearance, &tempignore);
    oproute_foreach_adjseg(r, adj, adjseg_check_serp_clearances, &tempignore);
    g_slist_free(tempignore);
  }
    

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
serpintine_insert_arc(toporouter_oproute_t *oproute, toporouter_serpintine_t *serp, gdouble x, gdouble y, gint wind, guint arcn)
{
  toporouter_vertex_t *newcentre = TOPOROUTER_VERTEX(gts_vertex_new(GTS_VERTEX_CLASS(toporouter_vertex_class()), x, y, vz(oproute->term1)));
  toporouter_arc_t *newarc = toporouter_arc_new(oproute, NULL, newcentre, serp->radius, wind);
  oproute->arcs = g_list_insert(oproute->arcs, newarc, arcn);
  serp->arcs = g_list_append(serp->arcs, newarc);
}

void
insert_serpintine(toporouter_t *r, toporouter_oproute_t *oproute, gdouble delta)
{
  guint nhalfcycles, arcn;
  gdouble length_required, x, y, halfa, x0, y0, x1, y1, curx, cury;
  gdouble px0, py0, px1, py1;
  gint curwind;
  gdouble perpgrad;
  gdouble radius = lookup_thickness(oproute->style) / 2. + lookup_keepaway(oproute->style) / 2.;
  toporouter_serpintine_t *serp;
  guint debug = 0;

  calculate_serpintine(delta, radius, r->serpintine_half_amplitude, &halfa, &nhalfcycles); 
  length_required = nhalfcycles*2.*radius + 2.*radius;

  find_serpintine_start(oproute, length_required, &x, &y, &arcn, &x0, &y0, &x1, &y1, debug);
 
  g_assert(x0 != x1 && y0 != y1);
  coord_move_towards_coord_values(x0, y0, x1, y1, 4. * radius, &x, &y);
  
  printf("\n\n*** TUNING %s ***\n\n", oproute->netlist);


  serp = toporouter_serpintine_new(x, y, x0, y0, x1, y1, NULL, halfa, radius, nhalfcycles);
  oproute->serp = serp;

  perpgrad = perpendicular_gradient(cartesian_gradient(x0, y0, x1, y1));

  coords_on_line(x, y, perpgrad, radius, &px0, &py0, &px1, &py1);
  curwind = coord_wind(x0, y0, x1, y1, px0, py0);
  
  serpintine_insert_arc(oproute, serp, px0, py0, curwind, arcn);
 
  curx = x; cury = y;

  for(guint i=0;i<nhalfcycles;i++) {
    gdouble px, py;

    g_assert(curx != x1 && cury != y1);
    coord_move_towards_coord_values(curx, cury, x1, y1, 2.*radius, &curx, &cury);
    coords_on_line(curx, cury, perpgrad, halfa-radius, &px0, &py0, &px1, &py1);
    if(coord_wind(x0, y0, x1, y1, px0, py0) == curwind) {
      px = px0; py = py0;
    }else{
      px = px1; py = py1;
    }

    serpintine_insert_arc(oproute, serp, px, py, -curwind, arcn+i+1);

    curwind = -curwind;
  }

  curwind = -curwind;

  g_assert(curx != x1 && cury != y1);
  coord_move_towards_coord_values(curx, cury, x1, y1, 2.*radius, &curx, &cury);
  coords_on_line(curx, cury, perpgrad, radius, &px0, &py0, &px1, &py1);
  if(coord_wind(x0, y0, x1, y1, px0, py0) == curwind) 
    serpintine_insert_arc(oproute, serp, px0, py0, curwind, arcn+nhalfcycles+1);
  else 
    serpintine_insert_arc(oproute, serp, px1, py1, curwind, arcn+nhalfcycles+1);

}

void
toporouter_oproute_find_adjacent_oproutes(toporouter_t *r, toporouter_oproute_t *oproute)
{
  GSList *i = oproute->path;

//  printf("FINDING ADJACENT OPROUTES FOR %s\n", oproute->netlist);

  if(oproute->adj) {
    g_slist_free(oproute->adj);
    oproute->adj = NULL;
  }

  while(i) {
    toporouter_vertex_t *v = TOPOROUTER_VERTEX(i->data);

    if(v->flags & VERTEX_FLAG_ROUTE) {
      GList *list = g_list_find(edge_routing(v->routingedge), v);
      if(list->next && !g_slist_find(oproute->adj, TOPOROUTER_VERTEX(list->next->data)->oproute)) { 
//        printf("\tfound %s\n", TOPOROUTER_VERTEX(list->next->data)->oproute->netlist);
        oproute->adj = g_slist_prepend(oproute->adj, TOPOROUTER_VERTEX(list->next->data)->oproute);
      }
      if(list->prev && !g_slist_find(oproute->adj, TOPOROUTER_VERTEX(list->prev->data)->oproute)) { 
//        printf("\tfound %s\n", TOPOROUTER_VERTEX(list->prev->data)->oproute->netlist);
        oproute->adj = g_slist_prepend(oproute->adj, TOPOROUTER_VERTEX(list->prev->data)->oproute);
      }
    }
    i = i->next;
  }

}

void
oproute_print_adjs(toporouter_oproute_t *oproute) 
{
  GSList *i = oproute->adj;

  printf("Adjacent oproutes:\n");
  while(i) {
    toporouter_oproute_t *a = TOPOROUTER_OPROUTE(i->data);
    printf("oproute %s\n", a->netlist);

    i = i->next;
  }


}



void
oproute_check_adj_serpintine_arcs(toporouter_oproute_t *oproute, toporouter_oproute_t *adj)
{
  GList *arcs = oproute->arcs;
  gdouble px = vx(oproute->term1), py = vy(oproute->term1);
  gdouble line_int_x, line_int_y;
  GList *serparcs;
  gdouble max = 0., max_x0, max_y0, max_x1, max_y1;
  toporouter_arc_t *maxarc = NULL;
  
  while(arcs) {
    toporouter_arc_t *arc = TOPOROUTER_ARC(arcs->data);

    serparcs = adj->serp ? adj->serp->arcs : NULL;
    while(serparcs) {
      toporouter_arc_t *serparc = TOPOROUTER_ARC(serparcs->data);
      
      if(vertex_line_normal_intersection(px, py, arc->x0, arc->y0, vx(serparc->centre), vy(serparc->centre), &line_int_x, &line_int_y)) {
        gdouble ms = oproute_min_spacing(adj, oproute) + serparc->r;
        gdouble d = sqrt(pow(line_int_x-vx(serparc->centre),2)+pow(line_int_y-vy(serparc->centre),2));

        if(d < ms) {
          //printf("\tfailure with serparc->centre %f,%f and line %f,%f %f,%f\n", vx(serparc->centre), vy(serparc->centre), px, py, arc->x0, arc->y0);
          if(!maxarc || d < max) {
            maxarc = serparc;
            max_x0 = px; max_y0 = py;
            max_x1 = arc->x0; max_y1 = arc->y0;
            max = d;
          }

        }
      }

      serparcs = serparcs->next;
    }


    px = arc->x1;
    py = arc->y1;
    arcs = arcs->next;
  }

  serparcs = adj->serp ? adj->serp->arcs : NULL;
  while(serparcs) {
    toporouter_arc_t *serparc = TOPOROUTER_ARC(serparcs->data);

    if(vertex_line_normal_intersection(px, py, vx(oproute->term2), vy(oproute->term2), vx(serparc->centre), vy(serparc->centre), &line_int_x, &line_int_y)) {
      gdouble ms = oproute_min_spacing(adj, oproute) + serparc->r;
      gdouble d = sqrt(pow(line_int_x-vx(serparc->centre),2)+pow(line_int_y-vy(serparc->centre),2));

      if(d < ms) {
        //printf("\tfailure with serparc->centre %f,%f and line %f,%f %f,%f\n", vx(serparc->centre), vy(serparc->centre), px, py, vx(oproute->term2), vy(oproute->term2));
        if(!maxarc || d < max) {
          maxarc = serparc;
          max_x0 = px; max_y0 = py;
          max_x1 = vx(oproute->term2); max_y1 = vy(oproute->term2);
          max = d;
        }

      }
    }

    serparcs = serparcs->next;
  }


  if(maxarc) {
    printf("SERPARC FAILURE max = %f serparc->centre = %f,%f\n",
        max, vx(maxarc->centre), vy(maxarc->centre));
  }

}

void
oproute_check_adjs(toporouter_t *r, toporouter_oproute_t *oproute, toporouter_oproute_t *dontcheck)
{
  GSList *i = oproute->adj;

  printf("checking oproute %s..\n", oproute->netlist);
  while(i) {
    toporouter_oproute_t *adj = TOPOROUTER_OPROUTE(i->data);
    gdouble px = vx(adj->term1), py = vy(adj->term1);
    GList *adjarcs = adj->arcs, *arcs;
    toporouter_vertex_t *pv;
    guint arcn = 0;
    guint recalculate = 0;
    
    if(adj == dontcheck) {
      i = i->next;
      continue;
    }

    //oproute_check_adj_serpintine_arcs(oproute, adj);
//    oproute_foreach_adjseg(oproute, adjseg_check_serp_clearances);
    
//    printf("\tchecking adj oproute %s..\n", adj->netlist);
//    print_oproute(adj);

    while(adjarcs) {
      toporouter_arc_t *adjarc = TOPOROUTER_ARC(adjarcs->data);
      
      pv = oproute->term1;
      arcs = oproute->arcs;
      while(arcs) {
        toporouter_arc_t *arc = TOPOROUTER_ARC(arcs->data);
        gdouble line_int_x, line_int_y;
//        toporouter_vertex_t *nv = arcs->next ? TOPOROUTER_ARC(arcs->next->data)->centre : oproute->term2;
/*
        if((vx(arc->centre) == px && vy(arc->centre) == py) || (vx(arc->centre) == adjarc->x0 && vy(arc->centre) == adjarc->y0)) {
          pv = arc->centre;
          arcs = arcs->next;
          continue;
        }
        
        if(arc->centre == adjarc->centre) {
          pv = arc->centre;
          arcs = arcs->next;
          continue;
        }
*/
        if(!path_adjacent_to_vertex(adj->path, arc->centre))
        if(vertex_line_normal_intersection(px, py, adjarc->x0, adjarc->y0, vx(arc->centre), vy(arc->centre), &line_int_x, &line_int_y)) {
//          if(vertex_wind(GTS_VERTEX(pv), GTS_VERTEX(nv), GTS_VERTEX(arc->centre)) == coord_wind(vx(pv), vy(pv), vx(nv), vy(nv), line_int_x, line_int_y)) {
            gdouble ms = oproute_min_spacing(adj, oproute) + arc->r;
            gdouble d = sqrt(pow(line_int_x-vx(arc->centre),2)+pow(line_int_y-vy(arc->centre),2));

            if(d < ms) {
              printf("\tfailure with arc->centre %f,%f and line %f,%f %f,%f\n",
                  vx(arc->centre), vy(arc->centre), px, py, adjarc->x0, adjarc->y0);

              oproute_insert_arc(adj, arcn++, arc, px, py, adjarc->x0, adjarc->y0, line_int_x, line_int_y, ms);
              recalculate = 1;
//            }
          }
              
        }

        pv = arc->centre;
        arcs = arcs->next;
      }

      arcn++;

      px = adjarc->x1;
      py = adjarc->y1;

      adjarcs = adjarcs->next;
    }
   
    pv = oproute->term1;
    arcs = oproute->arcs;
    while(arcs) {
      toporouter_arc_t *arc = TOPOROUTER_ARC(arcs->data);
      gdouble line_int_x, line_int_y;
//      toporouter_vertex_t *nv = arcs->next ? TOPOROUTER_ARC(arcs->next->data)->centre : oproute->term2;
/*      
      if((vx(arc->centre) == px && vy(arc->centre) == py) || (vx(arc->centre) == vx(adj->term2) && vy(arc->centre) == vy(adj->term2))) {
        pv = arc->centre;
        arcs = arcs->next;
        continue;
      }
*/
      if(!path_adjacent_to_vertex(adj->path, arc->centre))
      if(vertex_line_normal_intersection(px, py, vx(adj->term2), vy(adj->term2), vx(arc->centre), vy(arc->centre), &line_int_x, &line_int_y)) {
//        if(vertex_wind(GTS_VERTEX(pv), GTS_VERTEX(nv), GTS_VERTEX(arc->centre)) == coord_wind(vx(pv), vy(pv), vx(nv), vy(nv), line_int_x, line_int_y)) {
          gdouble ms = oproute_min_spacing(adj, oproute) + arc->r;
          gdouble d = sqrt(pow(line_int_x-vx(arc->centre),2)+pow(line_int_y-vy(arc->centre),2));
          if(d < ms) {
            printf("\t\tfailure with arc->centre %f,%f and line %f,%f %f,%f\n",
                vx(arc->centre), vy(arc->centre), px, py, vx(adj->term2), vy(adj->term2));
            oproute_insert_arc(adj, arcn++, arc, px, py, vx(adj->term2), vy(adj->term2), line_int_x, line_int_y, ms);
            recalculate = 1;
//          }
        }

      }

      pv = arc->centre;

      arcs = arcs->next;
    }
 
    if(recalculate) {
      fix_colinear_oproute_arcs(adj);
      calculate_oproute(r, adj);
      fix_overshoot_oproute_arcs(adj);
      calculate_oproute(r, adj);
      //oproute_check_adjs(r, adj, oproute);
    }

    i = i->next;
  }


}

void
oproutes_tof_match(toporouter_t *r, GSList *oproutes)
{
  guint n = g_slist_length(oproutes);
  GPtrArray *ranked_oproutes = g_ptr_array_sized_new(n);
  GSList *i = oproutes;
  gdouble critical;
  guint counter = 0;

  if(!n) return;

  while(i) {
    oproute_calculate_tof(TOPOROUTER_OPROUTE(i->data));
    toporouter_oproute_find_adjacent_oproutes(r, TOPOROUTER_OPROUTE(i->data));
    toporouter_oproute_calculate_clearances(TOPOROUTER_OPROUTE(i->data));
    g_ptr_array_add(ranked_oproutes, TOPOROUTER_OPROUTE(i->data));    
    i = i->next;
  }

  g_ptr_array_sort(ranked_oproutes, (GCompareFunc) oproute_tof_compare);

  critical = TOPOROUTER_OPROUTE(g_ptr_array_index(ranked_oproutes, n-1))->tof;

  printf("OPROUTE RANKINGS:\n");
  printf("%15s\t%15s\t%15s\t%15s\n", "TOF", "Delta", "Arcs", "Netlist");
  printf("---------------\t---------------\t---------------\n");
  
  for(toporouter_oproute_t **i = (toporouter_oproute_t **)ranked_oproutes->pdata; i < (toporouter_oproute_t **)ranked_oproutes->pdata + ranked_oproutes->len; i++) {
    print_oproute_tof(*i, critical);
  }
  printf("\n");
  
  for(toporouter_oproute_t **i = (toporouter_oproute_t **)ranked_oproutes->pdata; i < (toporouter_oproute_t **)ranked_oproutes->pdata + ranked_oproutes->len - 1; i++) {
    insert_serpintine(r, *i, critical - (*i)->tof);
    calculate_oproute(r, *i);
//    printf("\n\nOPROUTE AFTER insert\n");
//    print_oproute(*i);
//    oproute_check_adjs(r, *i, NULL);
    oproute_foreach_adjseg(r, *i, adjseg_check_clearance, NULL);
    oproute_foreach_adjseg(r, *i, adjseg_serpintine_update, NULL);
    oproute_foreach_adjseg(r, *i, adjseg_check_serp_clearances, NULL);
    counter++;
    if(counter==9) break;
  }
  printf("\n");


  g_ptr_array_free(ranked_oproutes, TRUE);

}

void
toporouter_export(toporouter_t *r) 
{
  GSList *i = r->paths;
  GSList *oproutes = NULL;

  while(i) {
    GSList *j = (GSList *)i->data;
    toporouter_oproute_t *oproute;
#ifdef DEBUG_EXPORT
    printf("length of path = %d\n", g_slist_length(j));
#endif    
    oproute = optimize_path(r, j);
    g_assert(oproute);
/*
  {
    int i;
    for(i=0;i<groupcount();i++) {
      char buffer[256];
      sprintf(buffer, "oppath%d.png", i);
      toporouter_draw_surface(r, r->layers[i].surface, buffer, 2048, 2048, 2, NULL, i, NULL);
    }
  }
*/ 

#ifdef DEBUG_EXPORT
    printf("OPROUTE:\n");
    print_oproute(oproute);
#endif    
    fix_colinear_oproute_arcs(oproute);

//    clean_oproute_arcs(oproute);
#ifdef DEBUG_EXPORT
    printf("OPROUTE AFTER CLEAN:\n");
    print_oproute(oproute);
#endif    
    
    calculate_oproute(r, oproute);
//    associate_oproute_and_vertices(oproute);
    
    fix_overshoot_oproute_arcs(oproute);
#ifdef DEBUG_EXPORT
    printf("OPROUTE AFTER FIX2:\n");
    print_oproute(oproute);
#endif    
    calculate_oproute(r, oproute);
   
    oproutes = g_slist_prepend(oproutes, oproute);


    i = i->next;
  }

///*

export_oproute_check:
  i = oproutes;
  while(i) {
    toporouter_oproute_t *oproute = TOPOROUTER_OPROUTE(i->data);
   
#ifdef DEBUG_EXPORT
    printf("CHECKING NETLIST %s\n", oproute->netlist);
#endif      

    if(check_oproute(r, oproute)) {
#ifdef DEBUG_EXPORT
      printf("CHECKFAIL NETLIST %s\n", oproute->netlist);
      printf("OPROUTE AFTER CHECKFAIL:\n");
      print_oproute(oproute);
      printf("\n\n");
#endif    
      fix_colinear_oproute_arcs(oproute);
      calculate_oproute(r, oproute);

      fix_overshoot_oproute_arcs(oproute);
#ifdef DEBUG_EXPORT
      printf("OPROUTE AFTER FIX:\n");
      print_oproute(oproute);
      printf("\n\n");
#endif    

      calculate_oproute(r, oproute);
      goto export_oproute_check;
    }
    
    i = i->next;
  }
//*/  

//  oproutes_tof_match(r, oproutes);
  
//export_oproute_finish:
  i = oproutes;
  while(i) {
    toporouter_oproute_t *oproute = (toporouter_oproute_t *) i->data;
    
    export_oproutes(r, oproute);
    
    oproute_free(oproute);
    
    i = i->next;
  }


  g_slist_free(oproutes);

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
  
  return routedata;
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

  return routedata;
}

void
delete_route(toporouter_route_t *routedata)
{
  GSList *i = routedata->path;
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
      gts_object_destroy ( GTS_OBJECT(tv) );
    }

    i = i->next;
  }

  if(routedata->path) g_slist_free(routedata->path);
  routedata->path = NULL;
  routedata->curpoint = NULL;
  routedata->score = INFINITY;
  routedata->alltemppoints = NULL;
}


/* remove route can be later reapplied */
void
remove_route(toporouter_route_t *routedata)
{
  GSList *i = routedata->path;

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
  GSList *i = routedata->path;
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
/*

void
print_nets_order(toporouter_t *r)
{
  GSList *i = r->nets;

  printf("Ordered two-net summary:\n");

  while(i) {
    toporouter_route_t *routedata = (toporouter_route_t *) i->data;
    toporouter_vertex_t *src = TOPOROUTER_VERTEX(routedata->src->point);
    toporouter_vertex_t *dest = NULL;
    gdouble mstdistance = 0., preroutedistance = 0.;

    g_assert( routedata->dests );

//     dests must be ordered 
    dest = TOPOROUTER_VERTEX(((toporouter_bbox_t*)routedata->dests->data)->point);

    mstdistance = gts_point_distance(GTS_POINT(src), GTS_POINT(dest));
    
    route(r, routedata, 0); 
//    clean_routing_edges(routedata); 
    preroutedistance = path_score(r, routedata->path);
    delete_route(routedata);

    
    printf("[RD %f,%f,%f -> %f,%f,%f MST:%f PREROUTE:%f]\n",
        vx(src), vy(src), vz(src),
        vx(dest), vy(dest), vz(dest),
        mstdistance, preroutedistance
        );

    i = i->next;
  }

}
void
print_nets(GSList *nets)
{
  GSList *i = nets;

  printf("Ordered two-nets:\n");

  while(i) {
    toporouter_route_t *routedata = (toporouter_route_t *) i->data;
    
    printf(" * %s\t", routedata->src->netlist);
    print_vertex(TOPOROUTER_VERTEX(routedata->src->point));

    i = i->next;
  }
  printf("\n");

}

void
order_nets_mst_ascending(toporouter_t *r) 
{
  // ascending length, with mst as the guiding topology 
  GSList *i = r->nets;
  GSList *newnets = NULL;
  while(i) {
    toporouter_route_t *routedata = (toporouter_route_t *) i->data;
    toporouter_vertex_t *src = TOPOROUTER_VERTEX(routedata->src->point);
    toporouter_vertex_t *dest = NULL;

    g_assert( routedata->dests );

    // dests must be ordered 
    dest = TOPOROUTER_VERTEX(((toporouter_bbox_t*)routedata->dests->data)->point);

    routedata->score = gts_point_distance(GTS_POINT(src), GTS_POINT(dest));

    newnets = g_slist_insert_sorted(newnets, routedata, compare_routedata_ascending);

    i = i->next;
  }
  
  g_slist_free(r->nets);
  r->nets = newnets;
}


void
order_nets_preroute_ascending(toporouter_t *r) 
{
  // ascending length, with preroute as the guiding topology 
  GSList *i = r->nets;
  GSList *newnets = NULL;
  while(i) {
    toporouter_route_t *routedata = (toporouter_route_t *) i->data;
    //toporouter_vertex_t *src = TOPOROUTER_VERTEX(routedata->src->point);
    //toporouter_vertex_t *dest = NULL;

    g_assert( routedata->dests );

    // dests must be ordered 
    //dest = TOPOROUTER_VERTEX(((toporouter_bbox_t*)routedata->dests->data)->point);
  
    route(r, routedata, 0); 

    delete_route(routedata);

    newnets = g_slist_insert_sorted(newnets, routedata, compare_routedata_ascending);

    i = i->next;
  }
  
  g_slist_free(r->nets);
  r->nets = newnets;
}

*/
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

  if(v) delete_route(routedata);

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

//      if(temproutedata->score - (*i)->score < 0) {
//        temproutedata->score = (*i)->score;
//      }

      if(!finite(temproutedata->score)) {
//        netscore->pairwise_detour[i - netscores_base] = INFINITY;
        netscore->pairwise_fails += 1;
      }else{
//        netscore->pairwise_detour[i - netscores_base] = temproutedata->score - (*i)->score;
        netscore->pairwise_detour_sum += temproutedata->score - (*i)->score;
      }

      delete_route(temproutedata);
    }else{
//      netscore->pairwise_detour[i - netscores_base] = 0.;
    }
    
  }

  delete_route(netscore->routedata);

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
order_nets_preroute_greedy(toporouter_t *r, GSList *nets, GSList **rnets) 
{
  GSList *i = nets;
  guint n = g_slist_length(nets);
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
    *rnets = g_slist_prepend(*rnets, (*i)->routedata);
    if(!finite((*i)->score)) failcount++;
    netscore_destroy(*i);
  }

  g_ptr_array_free(netscores, TRUE);

  return failcount;
}

  

void
create_pad_points(toporouter_t *r) 
{
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  GSList *i;

  i = r->nets;
  while(i) {
    toporouter_route_t *route = (toporouter_route_t *) i->data;
    GSList *path = route->path;

    toporouter_vertex_t *pv;
    
    if(path && path->next) {
      pv = TOPOROUTER_VERTEX(path->data);

//      printf("starting path @ %f,%f netlist = %s\n", vx(pv), vy(pv), vertex_netlist(pv));

      path = path->next;

      while(path) {
        toporouter_vertex_t *v = TOPOROUTER_VERTEX(path->data);
        toporouter_edge_t *e = v->routingedge;

//        printf("\tv %f,%f\n", vx(v), vy(v));

        if(TOPOROUTER_IS_CONSTRAINT(e)) {
          GSList *path2 = path->next;

//          printf("\t\tconstraint\n");

          while(path2) {
            toporouter_vertex_t *nv = TOPOROUTER_VERTEX(path2->data);
            toporouter_edge_t *ne = nv->routingedge;
            GList *nelist = ne ? g_list_find(edge_routing(ne), nv) : NULL;
            toporouter_vertex_t *commonv = NULL;
//            printf("\t\tnv %f,%f\n", vx(nv), vy(nv));
//            printf("\t\tpv %f,%f\n", vx(pv), vy(pv));

            if(ne && e) commonv = segment_common_vertex(GTS_SEGMENT(ne), GTS_SEGMENT(e));

            if(!commonv && (TOPOROUTER_IS_CONSTRAINT(ne) || 
                (nelist && (prev_lock2(nelist, ne, nv) || next_lock2(nelist, ne, nv))) ||
                !path2->next)) {
              gint nextwind;
              gdouble r;

//              if(!path2->next) 
//                nextwind = vertex_wind(GTS_VERTEX(nv), GTS_VERTEX(v), GTS_VERTEX(pv));
//              else
                nextwind = vertex_wind(GTS_VERTEX(pv), GTS_VERTEX(v), GTS_VERTEX(nv));

              r = (vertex_net_thickness(v) / 2.) + vertex_net_keepaway(v);


              if(nextwind == vertex_wind(GTS_VERTEX(pv), GTS_VERTEX(v), edge_v1(e))) {
                gdouble d = gts_point_distance(GTS_POINT(v), GTS_POINT(edge_v1(e)));

                toporouter_vertex_t *temp = TOPOROUTER_VERTEX( gts_vertex_new (vertex_class, vx(v), vy(v), vz(v)));
                temp->flags |= VERTEX_FLAG_FAKE;
                temp->boxes = TOPOROUTER_VERTEX(edge_v1(e))->boxes;
                vertex_move_towards_vertex(GTS_VERTEX(temp), edge_v1(e), r);
                v->fakev = temp;

                if(r > d) {
                  tedge_v1(e)->fakev = temp;
//                  printf("r %f > d %f\n", r, d);
                }
              }else{
                gdouble d = gts_point_distance(GTS_POINT(v), GTS_POINT(edge_v2(e)));
                // around v2 
                toporouter_vertex_t *temp = TOPOROUTER_VERTEX( gts_vertex_new (vertex_class, vx(v), vy(v), vz(v)));
                temp->flags |= VERTEX_FLAG_FAKE;
                temp->boxes = TOPOROUTER_VERTEX(edge_v2(e))->boxes;
                vertex_move_towards_vertex(GTS_VERTEX(temp), edge_v2(e), r);
                v->fakev = temp;

                if(r > d) {
                  tedge_v2(e)->fakev = temp;
//                  printf("r %f > d %f\n", r, d);
                }

              }

              break;
            } 

            path2 = path2->next;
          }


          pv = v;
        }else if(e) {
          GList *elist = g_list_find(edge_routing(e), v);
          toporouter_vertex_t *nv = path->next ? TOPOROUTER_VERTEX(path->next->data) : NULL, *commonv = NULL;

          toporouter_edge_t *ne = nv ? nv->routingedge : NULL;

          if(ne && TOPOROUTER_IS_CONSTRAINT(ne)) 
            commonv = segment_common_vertex(GTS_SEGMENT(ne), GTS_SEGMENT(e));

          if((prev_lock2(elist, e, v) || next_lock2(elist, e, v)) && !commonv) {

            pv = v;
          }

        }


        path = path->next;
      }
    }
    i = i->next;
  }

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
route_clusters_merge(toporouter_t *r, toporouter_route_t *routedata)
{
  GSList *i = routedata->dest->i;

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
  
  routedata->src->i = g_slist_concat(routedata->src->i, routedata->dest->i);

  free(routedata->dest);
  routedata->dest = routedata->src;

}

guint
hard_router(toporouter_t *r)
{
  GSList *i;
  guint failcount = 0, ncount = 0;
  toporouter_vertex_t *destv = NULL;
  order_nets_preroute_greedy(r, r->nets, &i); 

  while(i) {
    toporouter_route_t *data = (toporouter_route_t *)i->data; 
   
    printf("%d remaining, %d routed, %d failed\n", 
        g_slist_length(i),
        g_slist_length(r->routednets),
        g_slist_length(r->failednets));

    if((destv = route(r, data, 1))) { 
      GSList *path = split_path(data->path);//, *j = i->next; 

      r->paths = g_slist_concat(r->paths, path);
      r->routednets = g_slist_prepend(r->routednets, data);
      
      route_clusters_merge(r, data);

    }else{
      r->failednets = g_slist_prepend(r->failednets, data);
      failcount++;
      g_slist_free(data->path);
      data->path = NULL;
    }

    if(ncount++ >= r->effort) { 
      order_nets_preroute_greedy(r, i->next, &i);
      ncount = 0;
    } else {
      i = i->next;
    }
  } 

  return failcount;
}

guint
soft_router(toporouter_t *r)
{
  GSList *i;
  toporouter_vertex_t *destv = NULL;
  guint failcount = 0;
  order_nets_preroute_greedy(r, r->nets, &i); 

  printf("%d nets to route\n", g_slist_length(i));

  while(i) {
    toporouter_route_t *data = (toporouter_route_t *)i->data; 
    
    if((destv = route(r, data, 1))) { 
      GSList *path = split_path(data->path);//, *j = i->next;

      r->paths = g_slist_concat(r->paths, path);
      r->routednets = g_slist_prepend(r->routednets, data);

      route_clusters_merge(r, data);
      printf(".");

    }else{
      r->failednets = g_slist_prepend(r->failednets, data);
      failcount++;
    }


    i = i->next;
  } 

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
      r->keepoutlayers = g_slist_prepend(r->keepoutlayers, layer);
    }else if(!strcmp(argv[i], "no2")) {
      r->netsort = netscore_pairwise_size_compare;
    }else if(!strcmp(argv[i], "hardsrc")) {
      r->flags |= TOPOROUTER_FLAG_HARDSRC;
    }else if(!strcmp(argv[i], "harddest")) {
      r->flags |= TOPOROUTER_FLAG_HARDDEST;
    }else if(sscanf(argv[i], "h%d", &tempint)) {
      r->router = hard_router;
      r->effort = tempint;
    }
  }

}

void
finalize_path(toporouter_t *r, GSList *routedatas)
{
  while(routedatas) {
    toporouter_route_t *routedata = (toporouter_route_t *)routedatas->data;
    GSList *path = split_path(routedata->path); 

    r->paths = g_slist_concat(r->paths, path);

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
      if(routedata) r->nets = g_slist_prepend(r->nets, routedata); 
    }
  }
  END_LOOP;
//  /*
  if(!g_slist_length(r->nets)) {
    RAT_LOOP(PCB->Data);
    {
      toporouter_route_t *routedata = route_ratline(r, line);
      if(routedata) r->nets = g_slist_prepend(r->nets, routedata); 
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

  printf("Routed %d of %d two-nets\n", g_slist_length(r->routednets), g_slist_length(r->nets));

/*
  {
    int i;
    for(i=0;i<groupcount();i++) {
      char buffer[256];
      sprintf(buffer, "route%d.png", i);
      toporouter_draw_surface(r, r->layers[i].surface, buffer, 1024, 1024, 2, NULL, i, NULL);
    }
  }
*/
  for(gint i=0;i<groupcount();i++) {
    gts_surface_foreach_edge(r->layers[i].surface, spread_edge, NULL);
  }
/* 
  {
    int i;
    for(i=0;i<groupcount();i++) {
      char buffer[256];
      sprintf(buffer, "spread%d.png", i);
      toporouter_draw_surface(r, r->layers[i].surface, buffer, 2048, 2048, 2, NULL, i, NULL);
    }
  }
*/
  spring_embedder(r);
  create_pad_points(r);
/*
  {
    int i;
    for(i=0;i<groupcount();i++) {
      char buffer[256];
      sprintf(buffer, "spring%d.png", i);
      toporouter_draw_surface(r, r->layers[i].surface, buffer, 2048, 2048, 2, NULL, i, NULL);
    }
  }
*/ 
  toporouter_export(r);

/*
  {
    int i;
    for(i=0;i<groupcount();i++) {
      char buffer[256];
      sprintf(buffer, "export%d.png", i);
      toporouter_draw_surface(r, r->layers[i].surface, buffer, 2048, 2048, 2, NULL, i, NULL);
    }
  }
*/ 
  
//  fclose(r->debug);
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

