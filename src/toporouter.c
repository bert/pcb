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


#include "toporouter.h"

static void toporouter_edge_init (ToporouterEdge *edge)
{
  edge->netlist = NULL;
}

/**
 * toporouter_edge_class:
 *
 * Returns: the #ToporouterEdgeClass.
 */
ToporouterEdgeClass * 
toporouter_edge_class(void)
{
  static ToporouterEdgeClass *class = NULL;

  if (class == NULL) {
    GtsObjectClassInfo constraint_info = {
      "ToporouterEdge",
      sizeof (ToporouterEdge),
      sizeof (ToporouterEdgeClass),
      (GtsObjectClassInitFunc) NULL,
      (GtsObjectInitFunc) toporouter_edge_init,
      (GtsArgSetFunc) NULL,
      (GtsArgGetFunc) NULL
    };
    class = gts_object_class_new (GTS_OBJECT_CLASS (gts_edge_class ()), 
				  &constraint_info);
  }

  return class;
}

static void toporouter_bbox_init (ToporouterBBox *box)
{
  box->data = NULL;
  box->type = OTHER;
  box->constraints = NULL;
  box->netlist = NULL; 
  box->style = NULL;
}

/**
 * toporouter_bbox_class:
 *
 * Returns: the #ToporouterBBoxClass.
 */
ToporouterBBoxClass * 
toporouter_bbox_class(void)
{
  static ToporouterBBoxClass *class = NULL;

  if (class == NULL) {
    GtsObjectClassInfo constraint_info = {
      "ToporouterBBox",
      sizeof (ToporouterBBox),
      sizeof (ToporouterBBoxClass),
      (GtsObjectClassInitFunc) NULL,
      (GtsObjectInitFunc) toporouter_bbox_init,
      (GtsArgSetFunc) NULL,
      (GtsArgGetFunc) NULL
    };
    class = gts_object_class_new (GTS_OBJECT_CLASS (gts_bbox_class ()), 
				  &constraint_info);
  }

  return class;
}

static void toporouter_vertex_class_init (ToporouterVertexClass *klass)
{

}

static void toporouter_vertex_init (ToporouterVertex *vertex)
{
//  vertex->data = NULL;
//  vertex->type = TOPOROUTER_VERTEX_TYPE_NULL;
  vertex->boxes = NULL;
  vertex->parent = NULL;
  //vertex->visible = NULL;
  vertex->flags = 0;
  vertex->regions = NULL;
  vertex->zlink = NULL;

  vertex->corridors = NULL;
//  vertex->incident = NULL;
  vertex->attached = NULL;
}

/**
 * toporouter_vertex_class:
 *
 * Returns: the #ToporouterVertexClass.
 */
ToporouterVertexClass * 
toporouter_vertex_class(void)
{
  static ToporouterVertexClass *klass = NULL;

  if (klass == NULL) {
    GtsObjectClassInfo constraint_info = {
      "ToporouterVertex",
      sizeof (ToporouterVertex),
      sizeof (ToporouterVertexClass),
      (GtsObjectClassInitFunc) toporouter_vertex_class_init,
      (GtsObjectInitFunc) toporouter_vertex_init,
      (GtsArgSetFunc) NULL,
      (GtsArgGetFunc) NULL
    };
    klass = gts_object_class_new (GTS_OBJECT_CLASS (gts_vertex_class ()), 
				  &constraint_info);
  }

  return klass;
}

static void toporouter_constraint_class_init (ToporouterConstraintClass *klass)
{

}

static void toporouter_constraint_init (ToporouterConstraint *constraint)
{
//  constraint->data = NULL;
//  constraint->type = TOPOROUTER_CONSTRAINT_TYPE_NULL;
  constraint->box = NULL;
  //constraint->objectconstraints = NULL;
}

/**
 * toporouter_constraint_class:
 *
 * Returns: the #ToporouterConstraintClass.
 */
ToporouterConstraintClass * 
toporouter_constraint_class(void)
{
  static ToporouterConstraintClass *klass = NULL;

  if (klass == NULL) {
    GtsObjectClassInfo constraint_info = {
      "ToporouterConstraint",
      sizeof (ToporouterConstraint),
      sizeof (ToporouterConstraintClass),
      (GtsObjectClassInitFunc) toporouter_constraint_class_init,
      (GtsObjectInitFunc) toporouter_constraint_init,
      (GtsArgSetFunc) NULL,
      (GtsArgGetFunc) NULL
    };
    klass = gts_object_class_new (GTS_OBJECT_CLASS (gts_constraint_class ()), 
				  &constraint_info);
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

#ifdef CAIRO_H  
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
#ifdef CAIRO_H  
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
    if(!strcmp(style->Name, name)) return style->Keepaway;
  }
  END_LOOP;
  return Settings.Keepaway;
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

gint
toporouter_draw_vertex(gpointer item, gpointer data)
{
#ifdef CAIRO_H
  drawing_context_t *dc = (drawing_context_t *) data;
  ToporouterVertex *tv;
  PinType *pin;
  PadType *pad;
  gdouble blue;  
  
  if(TOPOROUTER_IS_VERTEX((GtsObject*)item)) {
    tv = TOPOROUTER_VERTEX((GtsObject*)item);
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
#ifdef CAIRO_H
  drawing_context_t *dc = (drawing_context_t *) data;
  ToporouterEdge *te;
  ToporouterConstraint *tc;
  
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
          break;
      }
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

void
toporouter_draw_surface(GtsSurface *s, char *filename, int w, int h, int mode, void *data) 
{
  drawing_context_t *dc;
  
  dc = toporouter_output_init(w, h, filename);
  dc->mode = mode;
  dc->data = data;

  gts_surface_foreach_edge(s, toporouter_draw_edge, dc);
  gts_surface_foreach_vertex(s, toporouter_draw_vertex, dc);

  if(mode == 1) {
    toporouter_route_t *routedata = (toporouter_route_t *)data;
//    GSList *i = routedata->src_points;
    ToporouterVertex *tv;
    ToporouterVertex *tv2;
/*
    while(i) {
      toporouter_draw_vertex(i->data, dc);
      i = i->next;
    }
    i = routedata->dest_points;
    while(i) {
      toporouter_draw_vertex(i->data, dc);
      i = i->next;
    }
  */  
    cairo_set_source_rgba(dc->cr, 0.0f, 1.0f, 0.0f, 0.8f);
    tv = routedata->curpoint;
    tv2 = tv->parent;
    while(tv2) {
      cairo_move_to(dc->cr, 
          GTS_POINT(tv)->x * dc->s + MARGIN, 
          GTS_POINT(tv)->y * dc->s + MARGIN);
      cairo_line_to(dc->cr, 
          GTS_POINT(tv2)->x * dc->s + MARGIN, 
          GTS_POINT(tv2)->y * dc->s + MARGIN);
      cairo_stroke(dc->cr);

      tv = tv->parent;
      tv2 = tv2->parent;

    }

  }else if(mode == 2) {
    toporouter_route_t *routedata = (toporouter_route_t *)data;
    ToporouterVertex *tv = NULL, *tv2 = NULL;
    GSList *i = routedata->path;
    

    while(i) {
      tv = TOPOROUTER_VERTEX(i->data);
      if(tv && tv2) {
        GSList *j = g_hash_table_lookup(tv->corridors, tv2);

        cairo_set_source_rgba(dc->cr, 0.0f, 1.0f, 0.0f, 0.8f);
        cairo_move_to(dc->cr, 
            GTS_POINT(tv)->x * dc->s + MARGIN, 
            GTS_POINT(tv)->y * dc->s + MARGIN);
        cairo_line_to(dc->cr, 
            GTS_POINT(tv2)->x * dc->s + MARGIN, 
            GTS_POINT(tv2)->y * dc->s + MARGIN);
        cairo_stroke(dc->cr);

        while(j) {
          GtsSegment *tempseg = GTS_SEGMENT(j->data);

          cairo_set_source_rgba(dc->cr, 0.0f, 0.0f, 1.0f, 0.4f);
          cairo_move_to(dc->cr, 
              GTS_POINT(tempseg->v1)->x * dc->s + MARGIN, 
              GTS_POINT(tempseg->v1)->y * dc->s + MARGIN);
          cairo_line_to(dc->cr, 
              GTS_POINT(tempseg->v2)->x * dc->s + MARGIN, 
              GTS_POINT(tempseg->v2)->y * dc->s + MARGIN);
          cairo_stroke(dc->cr);

          j = j->next;
        }
        

      }

      tv2 = tv;
      i = i->next;
    }

  }

  toporouter_output_close(dc);
}

toporouter_t *toporouter_new() 
{
  toporouter_t *r = calloc(1, sizeof(toporouter_t));
 
  r->layers = NULL;
  r->flags = 0;
  r->viamax     = 3;
  r->viacost    = 1000.;
  r->stublength = 300.;

  r->bboxes = NULL;
  r->bboxtree = NULL;

  return r;
}

void
toporouter_layer_free(toporouter_layer_t *l) 
{
  g_slist_free(l->vertices);
  g_slist_free(l->constraints);

}

void
toporouter_free(toporouter_t *r)
{
  int i;
  for(i=0;i<PCB->Data->LayerN;i++) {
    toporouter_layer_free(&r->layers[i]);
  }

  free(r->layers);  
  free(r);
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
print_toporouter_constraint(ToporouterConstraint *tc) 
{
  printf("%f,%f -> %f,%f ", 
      tc->c.edge.segment.v1->p.x,
      tc->c.edge.segment.v1->p.y,
      tc->c.edge.segment.v2->p.x,
      tc->c.edge.segment.v2->p.y);
}

inline void
print_toporouter_vertex(ToporouterVertex *tv) 
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

  gdouble ua_top = ((d->p.x - c->p.x) * (a->p.y - c->p.y)) - 
                   ((d->p.y - c->p.y) * (a->p.x - c->p.x));
  gdouble ua_bot = ((d->p.y - c->p.y) * (b->p.x - a->p.x)) - 
                   ((d->p.x - c->p.x) * (b->p.y - a->p.y));
  gdouble ua = ua_top / ua_bot;
  gdouble rx = a->p.x + (ua * (b->p.x - a->p.x));
  gdouble ry = a->p.y + (ua * (b->p.y - a->p.y));

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

  *t = gts_triangle_enclosing (gts_triangle_class (), vertices, 1000.0f);
  gts_triangle_vertices (*t, &v1, &v2, &v3);
  
  *surface = gts_surface_new (gts_surface_class (), gts_face_class (),
      GTS_EDGE_CLASS(toporouter_edge_class ()), GTS_VERTEX_CLASS(toporouter_vertex_class ()) );

  gts_surface_add_face (*surface, gts_face_new (gts_face_class (), (*t)->e1, (*t)->e2, (*t)->e3));
  
  i = vertices;
  while (i) {
    g_assert (gts_delaunay_add_vertex (*surface, i->data, NULL) == NULL);
    i = i->next;
  }
 /* 
  gts_allow_floating_vertices = TRUE;
  gts_object_destroy (GTS_OBJECT (v1));
  gts_object_destroy (GTS_OBJECT (v2));
  gts_object_destroy (GTS_OBJECT (v3));
  gts_allow_floating_vertices = FALSE;
*/
//  return surface;
}

ToporouterBBox *
toporouter_bbox_create(int layer, GSList *vertices, toporouter_term_t type, gpointer data)
{
  ToporouterBBox *bbox;
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
insert_constraint_edge_rec(toporouter_t *r, toporouter_layer_t *l, GtsVertex **v, ToporouterBBox *box)
{
  GtsEdgeClass *edge_class = GTS_EDGE_CLASS (toporouter_constraint_class ());
  GSList *i, *ii, *rlist = NULL, *tempconstraints;
  ToporouterConstraint *tc;
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
insert_vertex(toporouter_t *r, toporouter_layer_t *l, gdouble x, gdouble y, ToporouterBBox *box) 
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

  v = gts_vertex_new (vertex_class , x, y, 0.0f);
  TOPOROUTER_VERTEX(v)->boxes = g_slist_prepend(NULL, box);
  l->vertices = g_slist_prepend(l->vertices, v);

  return v;
}

GSList *
insert_constraint_edge(toporouter_t *r, toporouter_layer_t *l, gdouble x1, gdouble y1, guint flags1, gdouble x2, gdouble y2, guint flags2, ToporouterBBox *box)
{
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  GtsVertex *p[2];
  GtsVertex *v;
  GSList *i;

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
    p[0] = gts_vertex_new (vertex_class, x1, y1, 0.0f);
    TOPOROUTER_VERTEX(p[0])->boxes = g_slist_prepend(NULL, box);
    l->vertices = g_slist_prepend(l->vertices, p[0]);
  }
  if(p[1] == NULL) {
    p[1] = gts_vertex_new (vertex_class, x2, y2, 0.0f);
    TOPOROUTER_VERTEX(p[1])->boxes = g_slist_prepend(NULL, box);
    l->vertices = g_slist_prepend(l->vertices, p[1]);
  }

  TOPOROUTER_VERTEX(p[0])->flags = flags1;
  TOPOROUTER_VERTEX(p[1])->flags = flags2;
  
  return insert_constraint_edge_rec(r, l, p, box);
}

void
insert_constraints_from_list(toporouter_t *r, toporouter_layer_t *l, GSList *vlist, ToporouterBBox *box) 
{
  GSList *i = vlist;
  GtsPoint *curpoint = GTS_POINT(i->data);

  i = i->next;
  while(i) {
    box->constraints = g_slist_concat(box->constraints, 
        insert_constraint_edge(r, l, 
          curpoint->x, curpoint->y, TOPOROUTER_VERTEX(curpoint)->flags,
          GTS_POINT(i->data)->x, GTS_POINT(i->data)->y, TOPOROUTER_VERTEX(i->data)->flags,
          box));
    curpoint = GTS_POINT(i->data);
    i = i->next;
  }

  box->constraints = g_slist_concat(box->constraints, 
      insert_constraint_edge(r, l, 
        curpoint->x, curpoint->y, TOPOROUTER_VERTEX(curpoint)->flags,
        GTS_POINT(vlist->data)->x, GTS_POINT(vlist->data)->y, TOPOROUTER_VERTEX(vlist->data)->flags,
        box));

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
/*
GSList *
vertex_polygon_attachment_list(ToporouterBBox *b, toporouter_layer_t *l)
{
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  ToporouterVertex *v = TOPOROUTER_VERTEX(b->point);
  GSList *attach = NULL;
  GSList *i = b->constraints;
  gdouble tempwind;
  gdouble r1;
  gdouble r = (lookup_thickness(((PinType*)b->data)->Name) / 2.) 
    + lookup_keepaway(((PinType*)b->data)->Name);

  while(i) {
    GtsVertex* temp = gts_segment_midvertex(GTS_SEGMENT(i->data), vertex_class);
    gdouble m = segment_gradient(GTS_SEGMENT(i->data));
    ToporouterVertex *b0, *b1;
    gdouble seglength = gts_point_distance(GTS_POINT(GTS_SEGMENT(i->data)->v1), 
        GTS_POINT(GTS_SEGMENT(i->data)->v2));

    if(r >= seglength / 2.) {
      TOPOROUTER_VERTEX(GTS_SEGMENT(i->data)->v1)->flags |= VERTEX_FLAG_ATTACH;
      TOPOROUTER_VERTEX(GTS_SEGMENT(i->data)->v2)->flags |= VERTEX_FLAG_ATTACH;
      gts_object_destroy (GTS_OBJECT (temp));
      i = i->next;
      continue; 
    }

    b0 = TOPOROUTER_VERTEX( gts_vertex_new (vertex_class, 0., 0., 0.) ); 
    b1 = TOPOROUTER_VERTEX( gts_vertex_new (vertex_class, 0., 0., 0.) );

    points_on_line(GTS_POINT(temp), m, r, GTS_POINT(b0), GTS_POINT(b1));

    TOPOROUTER_VERTEX(b0)->flags = VERTEX_FLAG_ATTACH;
    TOPOROUTER_VERTEX(b1)->flags = VERTEX_FLAG_ATTACH;

    attach = g_slist_prepend(attach, b0);
    attach = g_slist_prepend(attach, b1);

    gts_object_destroy (GTS_OBJECT (temp));

    i = i->next;
  }

  return attach;
}
*/

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
    gdouble x3, gdouble y3)
{
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  GSList *r = NULL, *rr = NULL, *i;
  ToporouterVertex *curpoint, *temppoint;


  curpoint = TOPOROUTER_VERTEX(gts_vertex_new (vertex_class, x0, y0, 0.));

  r = g_slist_prepend(NULL, curpoint);
  r = g_slist_prepend(r, gts_vertex_new (vertex_class, x1, y1, 0.));
  r = g_slist_prepend(r, gts_vertex_new (vertex_class, x2, y2, 0.));
  r = g_slist_prepend(r, gts_vertex_new (vertex_class, x3, y3, 0.));
  
  i = r;
  while(i) {
    GtsPoint *midp = midpoint(GTS_POINT(i->data), GTS_POINT(curpoint));
    ToporouterVertex *b0;//, *b1;
//    gdouble seglength = gts_point_distance(GTS_POINT(i->data), GTS_POINT(curpoint));
//    gdouble m = cartesian_gradient( GTS_POINT(i->data)->x, GTS_POINT(i->data)->y,
//        GTS_POINT(curpoint)->x, GTS_POINT(curpoint)->y);

    temppoint = TOPOROUTER_VERTEX(i->data);
    rr = g_slist_prepend(rr, curpoint);
/*
    if(rad >= seglength / 3.) {
      curpoint->flags |= VERTEX_FLAG_ATTACH;
      temppoint->flags |= VERTEX_FLAG_ATTACH;
    }else{

      b0 = TOPOROUTER_VERTEX( gts_vertex_new (vertex_class, 0., 0., 0.) ); 
      b1 = TOPOROUTER_VERTEX( gts_vertex_new (vertex_class, 0., 0., 0.) );

      points_on_line(midp, m, rad, GTS_POINT(b0), GTS_POINT(b1));

      TOPOROUTER_VERTEX(b0)->flags |= VERTEX_FLAG_ATTACH;
      TOPOROUTER_VERTEX(b1)->flags |= VERTEX_FLAG_ATTACH;

      if(gts_point_distance(GTS_POINT(curpoint), GTS_POINT(b0)) < gts_point_distance(GTS_POINT(curpoint), GTS_POINT(b1))) {
        rr = g_slist_prepend(rr, b0);
        rr = g_slist_prepend(rr, b1);
      }else{
        rr = g_slist_prepend(rr, b1);
        rr = g_slist_prepend(rr, b0);
      }
    }
*/
    b0 = TOPOROUTER_VERTEX( gts_vertex_new (vertex_class, midp->x, midp->y, 0.) ); 
    TOPOROUTER_VERTEX(b0)->flags |= VERTEX_FLAG_ATTACH;
    //TOPOROUTER_VERTEX(b0)->attachcentre = centre;

    rr = g_slist_prepend(rr, b0);

    gts_object_destroy (GTS_OBJECT (midp));

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
read_pads(toporouter_t *r, toporouter_layer_t *l, int layer) 
{
  toporouter_spoint_t p[2], rv[5];
  gdouble x[2], y[2], t, m;
  GSList *objectconstraints;

  GSList *vlist = NULL;
  ToporouterBBox *bbox = NULL;

  int front = GetLayerGroupNumberByNumber (max_layer + COMPONENT_LAYER);
  int back = GetLayerGroupNumberByNumber (max_layer + SOLDER_LAYER);

  /* If its not the top or bottom layer, there are no pads to read */
  if(layer != front && layer != back) return 0;

  ELEMENT_LOOP(PCB->Data);
  {
    PAD_LOOP(element);
    {
      if( (layer == back && TEST_FLAG(ONSOLDERFLAG, pad)) || 
          (layer == front && !TEST_FLAG(ONSOLDERFLAG, pad)) ) {


        objectconstraints = NULL;
        t = (gdouble)pad->Thickness / 2.0f;
        x[0] = pad->Point1.X;
        x[1] = pad->Point2.X;
        y[0] = pad->Point1.Y;
        y[1] = pad->Point2.Y;

        if(r->flags & TOPOROUTER_FLAG_DEBUG_CDTS)
          printf("Pad P1=%f,%f P2=%f,%f\n\t ", x[0], y[0], x[1], y[1] ); 
       
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
                x[0]+t, y[0]-t);
            bbox = toporouter_bbox_create(layer, vlist, PAD, pad);
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
                rv[4].x, rv[4].y);
            bbox = toporouter_bbox_create(layer, vlist, PAD, pad);
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
                x[0]+t, y[0]-t);
            bbox = toporouter_bbox_create(layer, vlist, PAD, pad);
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
                rv[4].x, rv[4].y);
            bbox = toporouter_bbox_create(layer, vlist, PAD, pad);
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
  ToporouterBBox *bbox = NULL;

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
            x+t, y-t);
        bbox = toporouter_bbox_create(layer, vlist, PIN, pin);
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
            x+t, y-t);
        bbox = toporouter_bbox_create(layer, vlist, PIN, pin);
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
          x+t, y-t);
      bbox = toporouter_bbox_create(layer, vlist, VIA, via);
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
          x+t, y-t);
      bbox = toporouter_bbox_create(layer, vlist, VIA, via);
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
  ToporouterBBox *bbox = NULL;

  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());

  LINE_LOOP(layer);
  {
    xs[0] = line->Point1.X;
    xs[1] = line->Point2.X;
    ys[0] = line->Point1.Y;
    ys[1] = line->Point2.Y;
    if(!(xs[0] == xs[1] && ys[0] == ys[1])) { 
      vlist = g_slist_prepend(NULL, gts_vertex_new (vertex_class, xs[0], ys[0], 0.));
      vlist = g_slist_prepend(vlist, gts_vertex_new (vertex_class, xs[1], ys[1], 0.));
      bbox = toporouter_bbox_create(ln, vlist, LINE, line);
      r->bboxes = g_slist_prepend(r->bboxes, bbox);
      g_slist_free(vlist);
      
      bbox->constraints = g_slist_concat(bbox->constraints, insert_constraint_edge(r, l, xs[0], ys[0], 0, xs[1], ys[1], 0, bbox));
      printf("length of line constraints = %d\n", g_slist_length(bbox->constraints));
    }
  }
  END_LOOP;
  
  return 0;
}

int
read_board_constraints(toporouter_t *r, toporouter_layer_t *l, int layer) 
{
  GtsVertexClass *vertex_class = GTS_VERTEX_CLASS (toporouter_vertex_class ());
  GSList *vlist = NULL;
  ToporouterBBox *bbox = NULL;
  
  /* Add points for corners of board, and constrain those edges */
  vlist = g_slist_prepend(NULL, gts_vertex_new (vertex_class, 0., 0., 0.));
  vlist = g_slist_prepend(vlist, gts_vertex_new (vertex_class, PCB->MaxWidth, 0., 0.));
  vlist = g_slist_prepend(vlist, gts_vertex_new (vertex_class, PCB->MaxWidth, PCB->MaxHeight, 0.));
  vlist = g_slist_prepend(vlist, gts_vertex_new (vertex_class, 0., PCB->MaxHeight, 0.));
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
  gdouble min_edge = (PCB->Bloat + PCB->minWid) * 0.9;

//  fprintf(stderr, "quality = %f, area = %f \n", 
//      quality, area);
  if( gts_point_distance( &t->e1->segment.v1->p, &t->e1->segment.v2->p ) < min_edge )
    return 0.0;
  if( gts_point_distance( &t->e2->segment.v1->p, &t->e2->segment.v2->p ) < min_edge )
    return 0.0;
  if( gts_point_distance( &t->e3->segment.v1->p, &t->e3->segment.v2->p ) < min_edge )
    return 0.0;

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

  fprintf(stderr, "ADDED VERTICES\n");
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
      fprintf(r->debug, "edge p1=%f,%f p2=%f,%f\n", 
          temp->segment.v1->p.x,
          temp->segment.v1->p.y,
          temp->segment.v2->p.x,
          temp->segment.v2->p.y);

    g_assert (gts_delaunay_add_constraint (l->surface, i->data) == NULL);
    i = i->next;
  }
  fprintf(stderr, "ADDED CONSTRAINTS\n");
  gts_allow_floating_vertices = TRUE;
  gts_object_destroy (GTS_OBJECT (v1));
  gts_object_destroy (GTS_OBJECT (v2));
  gts_object_destroy (GTS_OBJECT (v3));
  gts_allow_floating_vertices = FALSE;
  /*
  {
    gpointer data[2];
    gdouble quality = 0.65, area = G_MAXDOUBLE;
    guint num = gts_delaunay_conform(l->surface, -1, (GtsEncroachFunc) gts_vertex_encroaches_edge, NULL);

    if (num == 0){
      data[0] = &quality;
      data[1] = &area;
      num = gts_delaunay_refine(l->surface, -1, (GtsEncroachFunc) gts_vertex_encroaches_edge, NULL, (GtsKeyFunc) triangle_cost, data);
    }
  }
  */
  gts_surface_print_stats(l->surface, stderr);
}
/*
void
insert_pad_stub_points(toporouter_t *r)
{
  GSList *i = r->bboxes, *j;
  ToporouterBBox *box;
  GtsSegment *seg;

  while(i) {
    box = TOPOROUTER_BBOX(i->data);
    if(box->constraints) {
      j = constraints;
      while(j) {
        seg = GTS_SEGMENT(j->data);

        j = j->next;
      }

    }
    i = i->next;
  }



}
*/

GtsTriangle* 
edge_triangle_oppisate(GtsEdge *e, GtsTriangle *t) 
{
//  g_assert(gts_triangle_neighbor_number(t) <= 1);

  int length =  g_slist_length(e->triangles);
  if(length > 2) printf("length = %d\n", length);
  
  if(e->triangles->data == t) {
    if(e->triangles->next) return GTS_TRIANGLE(e->triangles->next->data);
    return NULL;
  }
  return GTS_TRIANGLE(e->triangles->data);
}
 
struct visibility_data {
  GSList *visited;
  GSList *visible;

//  GSList *constraints;
  toporouter_layer_t *l;

  GtsVertex *origin;
};

/*
 * returns non-zero if point A can see B with constriant in way
 */
inline int
constraint_point_visible(ToporouterConstraint *c, GtsVertex *a, GtsVertex *b)
{
  int temp;
  int w1, w2;

  if(a == GTS_SEGMENT(c)->v1 || a == GTS_SEGMENT(c)->v2) return 1;

  w1 = vertex_wind(GTS_SEGMENT(c)->v1, GTS_SEGMENT(c)->v2, a);
  w2 = vertex_wind(GTS_SEGMENT(c)->v1, GTS_SEGMENT(c)->v2, b);

  if(w1 != 0 && w2 != 0 && w1 != w2) {

    if((temp = vertex_wind(a, GTS_SEGMENT(c)->v1, b)) == 0) return 0;

    if(temp == vertex_wind(a, GTS_SEGMENT(c)->v1, GTS_SEGMENT(c)->v2)) {

      if((temp = vertex_wind(a, GTS_SEGMENT(c)->v2, b)) == 0) return 0;

      if(temp == vertex_wind(a, GTS_SEGMENT(c)->v2, GTS_SEGMENT(c)->v1)) return 0;


    }
  }
  return 1; 
}

guint
point_visible(toporouter_layer_t *l, GtsVertex *a, GtsVertex *b)
{
  GSList *i = l->vertices;
  double d[3];

  while(i) {
    if(GTS_VERTEX(i->data) != a && GTS_VERTEX(i->data) != b)
    if(vertex_wind(a,b,GTS_VERTEX(i->data)) == 0) {
      d[0] = gts_point_distance(GTS_POINT(a), GTS_POINT(b));
      d[1] = gts_point_distance(GTS_POINT(b), GTS_POINT(i->data));
      d[2] = gts_point_distance(GTS_POINT(a), GTS_POINT(i->data));
      if(d[0] > d[1] && d[2] < d[0]) return 0;
    }
    i = i->next;
  }
  
  i = l->constraints;
  while(i) {
    if(!constraint_point_visible(TOPOROUTER_CONSTRAINT(i->data), a, b)){
      //printf("pointviz return 0\n");
      return 0;
    }
    i = i->next;
  }
  //printf("pointviz return 1\n");
  return 1;
}

void
compute_visibility_graph_rec(struct visibility_data *data, GtsEdge *e, GtsTriangle *t)
{
  GtsTriangle *cur = edge_triangle_oppisate(e, t);
  GtsVertex *ov;
  int viz[3];
  //printf("computer vizibility\n");
  if(cur == NULL) return;
  
  g_assert(t != cur);

  //g_tree_insert(data->visited, t, t);
  
  //if(g_tree_lookup(data->visited, cur)) return;
  
  if(g_slist_find(data->visited, cur)) return;
 
  data->visited = g_slist_prepend(data->visited, cur);
 
  ov = gts_triangle_vertex_opposite(cur, e);

  if((viz[0] = point_visible(data->l, GTS_SEGMENT(e)->v1, data->origin))) {
    data->visible = g_slist_insert_unique(data->visible, GTS_SEGMENT(e)->v1);
  }
  if((viz[1] = point_visible(data->l, GTS_SEGMENT(e)->v2, data->origin))) {
    data->visible = g_slist_insert_unique(data->visible, GTS_SEGMENT(e)->v2);
  }
  if((viz[2] = point_visible(data->l, ov, data->origin))) {
  //  if(ov != data->origin)
    data->visible = g_slist_insert_unique(data->visible, ov);
  }

  if(!viz[0] && !viz[1] && !viz[2]) return;
  
  if(cur->e1 != e) {
    if(!TOPOROUTER_IS_CONSTRAINT(cur->e1))
      compute_visibility_graph_rec(data, cur->e1, cur);
  }
  if(cur->e2 != e) {
    if(!TOPOROUTER_IS_CONSTRAINT(cur->e2))
      compute_visibility_graph_rec(data, cur->e2, cur);
  }
  if(cur->e3 != e) {
    if(!TOPOROUTER_IS_CONSTRAINT(cur->e3))
      compute_visibility_graph_rec(data, cur->e3, cur);
  }

}
gint
visited_cmp(gconstpointer a, gconstpointer b)
{
  if(a<b) return -1;
  if(a>b) return 1;
  return 0;
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

gint
point_angle_cmp(gconstpointer a, gconstpointer b, gpointer user_data) 
{
  gdouble ta = point_xangle(GTS_POINT(user_data), GTS_POINT(a));
  gdouble tb = point_xangle(GTS_POINT(user_data), GTS_POINT(b));
  if(ta < tb) return -1;
  if(ta > tb) return 1;
  printf("\nWARNING: point_angle_cmp: colinear\n");
  return 0;
}

toporouter_vertex_region_t *
vertex_region_new(GSList *points, ToporouterVertex *v1, ToporouterVertex *v2, ToporouterVertex *origin)
{
  toporouter_vertex_region_t *r = malloc(sizeof(toporouter_vertex_region_t));
  r->points = points;
  r->v1 = v1;
  r->v2 = v2;
  r->origin = origin;
  return r;
}

void
vertex_visibility_graph_old(ToporouterVertex *v, toporouter_layer_t *l)
{
  GSList *j, *i, *cp = NULL, *r = NULL;
  struct visibility_data data;

  //if(v->visible) g_slist_free(v->visible);

  data.visible = NULL;
  data.l = l;
  data.origin = GTS_VERTEX(v);

  data.visited = gts_vertex_triangles(GTS_VERTEX(v), NULL);
//  printf("%d starting triangles, ", g_slist_length(data.visited));
  j = data.visited;
  while(j) {
    compute_visibility_graph_rec(&data, 
        gts_triangle_edge_opposite(GTS_TRIANGLE(j->data), GTS_VERTEX(v)), 
        GTS_TRIANGLE(j->data));

    j = j->next; 
  }

//  v->removed = NULL;
  data.visible = g_slist_sort_with_data(data.visible, point_angle_cmp, v);

  //printf("%d triangles visited, %d visible, ", g_slist_length(data.visited),
 //     g_slist_length(data.visible));
  g_slist_free(data.visited);

  j = GTS_VERTEX(v)->segments;
  while(j) {
    if(TOPOROUTER_IS_CONSTRAINT(j->data)) { 
      ToporouterVertex *p;
      if(GTS_SEGMENT(j->data)->v1 == GTS_VERTEX(v))
        p = TOPOROUTER_VERTEX( GTS_SEGMENT(j->data)->v2 );
      else
        p = TOPOROUTER_VERTEX( GTS_SEGMENT(j->data)->v1 );
      cp = g_slist_prepend(cp, p);
    }
    j = j->next;
  }
  cp = g_slist_sort_with_data(cp, point_angle_cmp, v);

  if(g_slist_length(cp) == 0) {
    r = g_slist_prepend(NULL, vertex_region_new(data.visible, NULL, NULL, v));
  }else{
    ToporouterVertex *startc = TOPOROUTER_VERTEX( cp->data );
    toporouter_vertex_region_t *curregion;
    cp = g_slist_remove(cp, startc);

    curregion = vertex_region_new(g_slist_prepend(NULL, startc), startc, NULL, v);

    i = g_slist_find(data.visible, startc)->next;
    if(i == NULL) i = data.visible;
    while(TOPOROUTER_VERTEX(i->data) != startc) {
      ToporouterVertex *curc = TOPOROUTER_VERTEX(i->data);
      j = cp;
      while(j) {
        if(i->data == j->data) {
          curregion->v2 = TOPOROUTER_VERTEX(i->data);
          curregion->points = g_slist_insert_unique(curregion->points, curc);
          r = g_slist_prepend(r, curregion);
          curregion = vertex_region_new(g_slist_prepend(NULL, curc), curc, NULL, v);
          cp = g_slist_remove(cp, curc);
          goto visibility_cont_loop;
        }
        j = j->next;
      }
      curregion->points = g_slist_insert_unique(curregion->points, curc);

visibility_cont_loop:      
      i = i->next;
      if(i == NULL) i = data.visible;
    }
    curregion->v2 = startc;
    curregion->points = g_slist_insert_unique(curregion->points, startc);
    r = g_slist_prepend(r, curregion);

    g_slist_free(data.visible);
  }
//  r = g_slist_sort_with_data(r, point_angle_cmp, v);
  v->regions = r;

  //printf(" [REGIONS %d]\n", g_slist_length(r));

//  return data.visible;
}

GSList *
vertex_corridor_edges(ToporouterVertex *v, ToporouterVertex *dest)
{
  GSList *r = NULL, *i, *triangles;
  GtsSegment *s = gts_segment_new(gts_segment_class(), 
      gts_vertex_new(gts_vertex_class(), GTS_POINT(v)->x, GTS_POINT(v)->y, 0.),
      gts_vertex_new(gts_vertex_class(), GTS_POINT(dest)->x, GTS_POINT(dest)->y, 0.));

  triangles = gts_vertex_triangles(GTS_VERTEX(v), NULL);
  i = triangles;
  while(i) {
    GtsTriangle *curt = GTS_TRIANGLE(i->data);
    GtsEdge* e = gts_triangle_edge_opposite(curt, GTS_VERTEX(v));
    GtsIntersect intersect = gts_segments_are_intersecting(s,GTS_SEGMENT(e));
    
    if(intersect == GTS_IN) {
intersect_gts_in:      
      r = g_slist_prepend(r, e);
      curt = edge_triangle_oppisate(e, curt);
      
      if(curt->e1 != e) {
        intersect = gts_segments_are_intersecting(s,GTS_SEGMENT(curt->e1));
        if(intersect == GTS_IN) {
          e = curt->e1;
          goto intersect_gts_in;
        }else if(intersect == GTS_ON) {
          e = curt->e1;
          goto intersect_gts_on;
        }
      }
      if(curt->e2 != e) {
        intersect = gts_segments_are_intersecting(s,GTS_SEGMENT(curt->e2));
        if(intersect == GTS_IN) {
          e = curt->e2;
          goto intersect_gts_in;
        }else if(intersect == GTS_ON) {
          e = curt->e2;
          goto intersect_gts_on;
        }
      }
      if(curt->e3 != e) {
        intersect = gts_segments_are_intersecting(s,GTS_SEGMENT(curt->e3));
        if(intersect == GTS_IN) {
          e = curt->e3;
          goto intersect_gts_in;
        }else if(intersect == GTS_ON) {
          e = curt->e3;
          goto intersect_gts_on;
        }
      }

    }else if(intersect == GTS_ON) {
intersect_gts_on:
      if(GTS_SEGMENT(e)->v1 == GTS_VERTEX(dest) || GTS_SEGMENT(e)->v2 == GTS_VERTEX(dest)) {
        return r;
      }
      if(vertex_wind(GTS_VERTEX(v), GTS_SEGMENT(e)->v1, GTS_VERTEX(dest)) == 0) {
        return g_slist_concat(r, vertex_corridor_edges(TOPOROUTER_VERTEX(GTS_SEGMENT(e)->v1), dest));
      }else if(vertex_wind(GTS_VERTEX(v), GTS_SEGMENT(e)->v2, GTS_VERTEX(dest)) == 0) {
        return g_slist_concat(r, vertex_corridor_edges(TOPOROUTER_VERTEX(GTS_SEGMENT(e)->v2), dest));
      }
      printf("ERROR: vertex_corridor_edges: GTS_ON but no colinear points\n");
    }

    i = i->next; 
  }
  
  printf("ERROR: vertex_corridor_edges: no triangles had edges intersecting.. \n");
  return r;
}

void
vertex_visibility_graph(ToporouterVertex *v, toporouter_layer_t *l)
{
  GSList *i, *visible = NULL;
  GSList *j, *cp = NULL, *r = NULL;

  v->corridors = g_hash_table_new(g_direct_hash, g_direct_equal);
  
  i = l->vertices;
  while(i) {
    if(TOPOROUTER_VERTEX(i->data) != v) {
      if(point_visible(l, GTS_VERTEX(i->data), GTS_VERTEX(v))) {
        visible = g_slist_prepend(visible, TOPOROUTER_VERTEX(i->data));    
        g_hash_table_insert(v->corridors, i->data, vertex_corridor_edges(v, TOPOROUTER_VERTEX(i->data)));
      }
    }
    i = i->next;
  }

  visible = g_slist_sort_with_data(visible, point_angle_cmp, v);
//  printf("length of visible = %d\n", g_slist_length(visible));

  j = GTS_VERTEX(v)->segments;
  while(j) {
    if(TOPOROUTER_IS_CONSTRAINT(j->data)) { 
      ToporouterVertex *p;
      if(GTS_SEGMENT(j->data)->v1 == GTS_VERTEX(v))
        p = TOPOROUTER_VERTEX( GTS_SEGMENT(j->data)->v2 );
      else
        p = TOPOROUTER_VERTEX( GTS_SEGMENT(j->data)->v1 );
      cp = g_slist_prepend(cp, p);
    }
    j = j->next;
  }
  
  cp = g_slist_sort_with_data(cp, point_angle_cmp, v);

  if(g_slist_length(cp) == 0) {
    r = g_slist_prepend(NULL, vertex_region_new(visible, NULL, NULL, v));
  }else{
    ToporouterVertex *startc = TOPOROUTER_VERTEX( cp->data );
    toporouter_vertex_region_t *curregion;
    cp = g_slist_remove(cp, startc);

    curregion = vertex_region_new(g_slist_prepend(NULL, startc), startc, NULL, v);

    i = g_slist_find(visible, startc)->next;
    if(i == NULL) i = visible;
    while(TOPOROUTER_VERTEX(i->data) != startc) {
      ToporouterVertex *curc = TOPOROUTER_VERTEX(i->data);
      j = cp;
      while(j) {
        if(i->data == j->data) {
          curregion->v2 = TOPOROUTER_VERTEX(i->data);
          curregion->points = g_slist_insert_unique(curregion->points, curc);
          r = g_slist_prepend(r, curregion);
          curregion = vertex_region_new(g_slist_prepend(NULL, curc), curc, NULL, v);
          cp = g_slist_remove(cp, curc);
          goto temp_visibility_cont_loop;
        }
        j = j->next;
      }
      curregion->points = g_slist_insert_unique(curregion->points, curc);

temp_visibility_cont_loop:      
      i = i->next;
      if(i == NULL) i = visible;
    }
    curregion->v2 = startc;
    curregion->points = g_slist_insert_unique(curregion->points, startc);
    r = g_slist_prepend(r, curregion);

    g_slist_free(visible);
  }

//  r = g_slist_sort_with_data(r, point_angle_cmp, v);
  v->regions = r;
//  v->visible = visible;

//  return visible;
  
}

/*
 * returns > 0 if a is on b's box constraints 
 */
int
vertex_in_box_constraints(GtsVertex *v, ToporouterBBox *box)
{
  GSList *j;

  j = box->constraints;
  while(j) {
    GtsSegment *seg = GTS_SEGMENT(j->data);
    if(seg->v1 == v || seg->v2 == v)
      return 1;
    j = j->next;
  }

  return 0;
}

/*
 * in != 0 for going in, otherwise 0 for going out of polygon
 */


/*
 * returns a list of points which are candidates for attachment
 */
GSList *
vertex_attachment_list(ToporouterVertex *curpoint, ToporouterVertex *v, toporouter_route_t *data, toporouter_layer_t *l)
{
  GSList *attach = NULL;

  /* is the vertex on either src or dest bboxes? */
  if(vertex_in_box_constraints(GTS_VERTEX(v), data->src)) {

    printf("vertex in src box, thickness = %f keepaway = %f name = %s\n",
        lookup_thickness(((PinType*)data->src->data)->Name),
        lookup_keepaway(((PinType*)data->src->data)->Name),
        ((PinType*)data->src->data)->Name);
 //   return vertex_polygon_attachment_list(v, data->src, l); 
  }else if(vertex_in_box_constraints(GTS_VERTEX(v), data->dest)) {

    printf("vertex in dest box, thickness = %f keepaway = %f name = %s\n",
        lookup_thickness(((PinType*)data->dest->data)->Name),
        lookup_keepaway(((PinType*)data->dest->data)->Name),
        ((PinType*)data->dest->data)->Name);

//    return vertex_polygon_attachment_list(v, data->dest, l); 
  }else{




  }


  return attach;
}

void
compute_visibility_graph(toporouter_t *r)
{
  ToporouterVertex *v;
  GSList *i;
  toporouter_layer_t *cur_layer = r->layers;
  guint n;

  printf("COMPUTING VISIBILITY GRAPHS\n");

  for(n = 0; n < max_layer; n++) {
    i = cur_layer->vertices;
    while(i) {

      v = TOPOROUTER_VERTEX(i->data);
      
      vertex_visibility_graph(v, cur_layer);

      i = i->next;
    }

    cur_layer++;
  }

  cur_layer = r->layers;
  i = cur_layer->vertices;
  while(i) {
    v = TOPOROUTER_VERTEX(i->data);

    if(v->boxes) {
      if(TOPOROUTER_BBOX(v->boxes->data)->type == PIN || TOPOROUTER_BBOX(v->boxes->data)->type == VIA) {
        GSList *pins = g_slist_prepend(NULL, v);
        GSList *b = gts_bb_tree_stabbed(r->bboxtree, GTS_POINT(v));
        GSList *j = b;
        while(j) {
          ToporouterBBox *box = TOPOROUTER_BBOX(j->data);
          if(box->type == PIN || box->type == VIA) {
            TOPOROUTER_VERTEX(box->point)->zlink = pins;
            if(TOPOROUTER_VERTEX(box->point) != v) {
              pins = g_slist_append(pins, box->point);
            }
          }
          j = j->next;
        }
        g_slist_free(b);
      }
    } 
    i = i->next;
  }

  printf("finished COMPUTING VISIBILITY GRAPHS\n");
}


void
import_geometry(toporouter_t *r) 
{
  GSList *i;
  GtsVertex *v1, *v2, *v3;
  int j;
  char buffer[64];
  toporouter_layer_t *cur_layer;

 /* 
  printf("import board width = %d, board height = %d, layers = %d\n", 
      PCB->MaxWidth, PCB->MaxHeight, PCB->Data->LayerN);
  layercount = 0;
  LAYER_LOOP(PCB->Data, PCB->Data->LayerN);
  {
    printf("Layer %d:\n", layercount);
    linecount = 0;
    LINE_LOOP(layer);
    {
      linecount++;
    }
    END_LOOP;
    printf("\t%d lines\n", layer->LineN);
    printf("\t%d polygons\n", layer->PolygonN);
    
    layercount++;    
  }
  END_LOOP;
*/
  
  /* Allocate space for per layer struct */
  cur_layer = r->layers = malloc(PCB->Data->LayerN * sizeof(toporouter_layer_t));


 
//  int front = GetLayerGroupNumberByNumber (max_layer + COMPONENT_LAYER);
//  int back = GetLayerGroupNumberByNumber (max_layer + SOLDER_LAYER);

//  fprintf(stderr, "front = %d, back = %d\n", front, back);

  /* Foreach layer, read in pad vertices and constraints, and build CDT */
//  for(j=0;j<PCB->Data->LayerN;j++) {
  LAYER_LOOP(PCB->Data, max_layer)
  {
    j = GetLayerNumber(PCB->Data, layer);
//    if(r->flags & TOPOROUTER_FLAG_DEBUG_CDTS)
      printf("*** LAYER %d ***\n", j);
   
    // Copy slists containing vertices and constraints for pins, vias and board extents 
    cur_layer->vertices    = NULL;//g_slist_copy(vertices);
    cur_layer->constraints = NULL;//g_slist_copy(constraints); 
   
    printf("reading board constraints\n");
    read_board_constraints(r, cur_layer, j);
    printf("reading points\n");
    read_points(r, cur_layer, j);
    printf("reading pads\n");
    read_pads(r, cur_layer, j);
    printf("reading lines\n");
    read_lines(r, cur_layer, layer, j);
    printf("building CDT\n");
    build_cdt(r, cur_layer);

    if(r->flags & TOPOROUTER_FLAG_DUMP_CDTS) {
      sprintf(buffer, "cdt-layer%d.png", j);
      toporouter_draw_surface(r->layers[j].surface, buffer, 1280, 1280, 0, NULL);
/*      
      if((f=fopen(buffer, "w")) == NULL) {
        fprintf(stderr, "Error opening file %s for output\n", buffer);
      }else{
        gts_surface_write_oogl(r->layers[j].surface, f);
        fclose(f);
      }
*/
    }
    cur_layer++;
  }
  END_LOOP;
  

  i = r->bboxes;
  while(i) {
    gts_triangle_vertices(TOPOROUTER_BBOX(i->data)->enclosing, &v1, &v2, &v3);

    gts_allow_floating_vertices = TRUE;
    gts_object_destroy (GTS_OBJECT (v1));
    gts_object_destroy (GTS_OBJECT (v2));
    gts_object_destroy (GTS_OBJECT (v3));
    gts_allow_floating_vertices = FALSE;
    
    i = i->next;
  }

  r->bboxtree = gts_bb_tree_new(r->bboxes);
  
  {
    NetListListType nets = CollectSubnets(False);
    NETLIST_LOOP(&nets);
    {
      if(netlist->NetN > 0) {

        NET_LOOP(netlist);
        {
          GtsPoint *p = gts_point_new(gts_point_class(), net->Connection->X, net->Connection->Y, 0.);
          GSList *boxes = gts_bb_tree_stabbed(r->bboxtree, p);
          GSList *i = boxes;

          gts_object_destroy(GTS_OBJECT(p));

          while(i) {
            ToporouterBBox *box = TOPOROUTER_BBOX(i->data);
            if(box->type == PIN || box->type == PAD || box->type == VIA) {
              box->netlist = netlist->Net->Connection->menu->Name;
              box->style = netlist->Net->Connection->menu->Style;
            }
            i = i->next;
          }
          if(!boxes)
            printf("WARNING: no boxes found for netlist \"%s\"\n", netlist->Net->Connection->menu->Name);

          g_slist_free(boxes);

        }
        END_LOOP;

      }
    }
    END_LOOP;
    FreeNetListListMemory(&nets);
  }
  
  compute_visibility_graph(r);

  printf("finished import!\n");
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

ToporouterBBox *
find_routebox(toporouter_t *r, GtsPoint *p, int layer) 
{
  GSList *hits;
  ToporouterBBox *box;

  hits = gts_bb_tree_stabbed(r->bboxtree, p);

  while(hits) {
    box = TOPOROUTER_BBOX(hits->data);
    if(box->type != BOARD && box->layer == layer)
    if(gts_point_locate(p, box->surface, NULL)) {
      return box;
    }
    hits = hits->next;
  }
  return NULL;
}

static void
dump_routebox(ToporouterBBox *b)
{
  printf ("RB: (%f,%f)-(%f,%f) l%d; ",
          GTS_BBOX(b)->x1, GTS_BBOX(b)->y1,
          GTS_BBOX(b)->x2, GTS_BBOX(b)->y2,
          (int) b->layer);
  switch (b->type)
    {
    case PAD:
      printf ("PAD[%s %s]; ", ((PadType*)b->data)->Name, ((PadType*)b->data)->Number);
      break;
    case PIN:
      printf ("PIN[%s %s]; ", ((PinType*)b->data)->Name, ((PinType*)b->data)->Number);
      break;
    case VIA:
      printf ("VIA[%s %s]; ", ((PinType*)b->data)->Name, ((PinType*)b->data)->Number);
      break;
    case BOARD:
      printf ("BOARD; ");
      break;
    default:
      printf ("UNKNOWN; ");
      break;
    }
  
  if(b->constraints) 
    printf("CONSTRAINTS[%d]; ", g_slist_length(b->constraints));

  if(b->point) 
    printf("(has point); ");

  printf ("\n");
}


void
insert_joined_segments(
    GtsPoint *curpoint, 
    GtsPoint *destpoint, 
    GtsSegment *seg, 
    GSList **ccw_points, 
    GSList **cw_points, 
    GTree *seen_segs
    ) 
{
  GSList *i;
  GtsSegment *tempseg;
  int tempint;

  tempint = point_wind(curpoint, destpoint, GTS_POINT(seg->v1));

  if(tempint > 0) {
    *ccw_points = g_slist_insert_unique(*ccw_points, seg->v1);
    *cw_points = g_slist_insert_unique(*cw_points, seg->v2);
  }else if(tempint < 0) {
    *ccw_points = g_slist_insert_unique(*ccw_points, seg->v2);
    *cw_points = g_slist_insert_unique(*cw_points, seg->v1);
  }else{

    tempint = point_wind(curpoint, destpoint, GTS_POINT(seg->v2));

    if(tempint > 0) {
      *ccw_points = g_slist_insert_unique(*ccw_points, seg->v2);
      *cw_points = g_slist_insert_unique(*cw_points, seg->v1);
    }else if(tempint < 0) {
      *ccw_points = g_slist_insert_unique(*ccw_points, seg->v1);
      *cw_points = g_slist_insert_unique(*cw_points, seg->v2);
    }
  }

  g_tree_insert(seen_segs, seg, seg); 

  i = seg->v1->segments;
  while(i) {
    tempseg = GTS_SEGMENT(i->data);

    if(tempseg != seg && !g_tree_lookup(seen_segs, tempseg)) {

      if(TOPOROUTER_IS_CONSTRAINT(i->data)) {
        insert_joined_segments(curpoint, destpoint, tempseg, ccw_points, cw_points, seen_segs);
        return;
      }
      if(TOPOROUTER_IS_EDGE(i->data))
        if(TOPOROUTER_EDGE(i->data)->netlist) {
          insert_joined_segments(curpoint, destpoint, tempseg, ccw_points, cw_points, seen_segs);
          return;
        }
      
      g_tree_insert(seen_segs, tempseg, tempseg); 
    }
    i = i->next;
  }
  
  i = seg->v2->segments;
  while(i) {
    tempseg = GTS_SEGMENT(i->data);

    if(tempseg != seg && !g_tree_lookup(seen_segs, tempseg)) {

      if(TOPOROUTER_IS_CONSTRAINT(i->data)) {
        insert_joined_segments(curpoint, destpoint, tempseg, ccw_points, cw_points, seen_segs);
        return;
      }
      if(TOPOROUTER_IS_EDGE(i->data))
        if(TOPOROUTER_EDGE(i->data)->netlist) {
          insert_joined_segments(curpoint, destpoint, tempseg, ccw_points, cw_points, seen_segs);
          return;
        }
      
      g_tree_insert(seen_segs, tempseg, tempseg); 
    }
    i = i->next;
  }

  return;
}

// points should be a copy
GSList *
make_chain(GtsPoint *curpoint, GtsPoint *destpoint, GSList *points, int dir) 
{
  GSList *chain = NULL;
  GSList *i, *j;
  GtsPoint *prevpoint;
  int tempdir;

  chain = g_slist_prepend(chain, destpoint);
  prevpoint = destpoint;

make_chain_next_point:  
  i = points;
  while(i) {

    j = points;
    while(j) {
      if(j != i) {
        tempdir = point_wind(prevpoint, GTS_POINT(i->data), GTS_POINT(j->data));
        if(tempdir != dir && tempdir != 0)
          goto make_chain_next_i;

      }
      j = j->next;
    }

    chain = g_slist_prepend(chain, i->data);
    points = g_slist_remove(points, i->data);
    prevpoint = GTS_POINT(i->data);
    goto make_chain_next_point;

make_chain_next_i:
    i = i->next;
  }

  chain = g_slist_prepend(chain, curpoint);
  g_slist_free(points);

  return chain;
}

gdouble 
chain_length(GSList *chain)
{
  GSList *i;
  GtsPoint *curpoint, *prevpoint;
  gdouble r = 0.;

  i = chain;
  prevpoint = GTS_POINT(i->data);
  i = i->next;
  while(i) {
    curpoint = GTS_POINT(i->data);
    r += gts_point_distance(curpoint, prevpoint);
    prevpoint = curpoint;
    i = i->next;
  }

  return r;
}

/*
 * This calculates the "h"-cost, or heuristic cost, for use in the
 * A* path finding algorithm. 
 */
gdouble
h_cost(toporouter_layer_t *l, GtsPoint *curpoint, GtsPoint *destpoint) 
{
  GSList *i;
  GTree *seen_segs;
  GSList *ccw_points, *cw_points;
  GSList *ccw_chain, *cw_chain;
  gdouble r;

  if(curpoint == destpoint) return 0.;
  
  seen_segs = g_tree_new(compare_segments);
  ccw_points = cw_points = NULL;

  /* TODO: could also check for not enough edge capacity at this point */
  i = l->constraints;
  while(i) {
    if(vertex_intersect_prop(
          GTS_SEGMENT(i->data)->v1,
          GTS_SEGMENT(i->data)->v2, 
          GTS_VERTEX(curpoint), 
          GTS_VERTEX(destpoint))
        ) 
      insert_joined_segments(curpoint, destpoint, GTS_SEGMENT(i->data), &ccw_points, &cw_points, seen_segs);
    i = i->next;
  }

  i = l->edges;
  while(i) {
    if(vertex_intersect_prop(
          GTS_SEGMENT(i->data)->v1,
          GTS_SEGMENT(i->data)->v2, 
          GTS_VERTEX(curpoint), 
          GTS_VERTEX(destpoint))
        ) 
      insert_joined_segments(curpoint, destpoint, GTS_SEGMENT(i->data), &ccw_points, &cw_points, seen_segs);
    i = i->next;
  }

  ccw_chain = make_chain(curpoint, destpoint, g_slist_copy(ccw_points), 1); 
  cw_chain = make_chain(curpoint, destpoint, g_slist_copy(cw_points), -1); 
  
  r = MIN(chain_length(ccw_chain), chain_length(cw_chain));

  /* TODO: this heuristic could be improved by recursively checking bits which
   * would break convexity with curpoint and destpoint
   */

  g_slist_free(ccw_chain);
  g_slist_free(cw_chain);
  g_slist_free(ccw_points);
  g_slist_free(cw_points);
  g_tree_destroy(seen_segs);

  return r;
}

/*
 * This calculates the "g"-cost, or generated cost (the cost of the path
 * so far), for use in the A* path finding algorithm.
 */
/*
gdouble
g_cost(GSList *closelist, GtsPoint *curpoint) 
{
  GSList *i;
  GtsPoint *temppoint;

  gdouble r = 0;

  i = closelist;
  while(i) {
    temppoint = GTS_POINT(i->data);
    
    r += gts_point_distance(temppoint, curpoint);
    
    curpoint = temppoint;

    i = i->next;
  }

  return r;
}
*/
gdouble
g_cost(ToporouterVertex *curpoint) 
{
  gdouble r = 0.;
  while(curpoint) {
    r += gts_point_distance(GTS_POINT(curpoint), GTS_POINT(curpoint->parent));
    curpoint = curpoint->parent;
  }
  return r;
}

gdouble
simple_h_cost(ToporouterVertex *curpoint, ToporouterVertex *destpoint)
{
  return gts_point_distance(GTS_POINT(curpoint), GTS_POINT(destpoint));
}


/*
 * This calculates the "f"-cost, which = g + h
 * so far), for use in the A* path finding algorithm.
 */
/*
gdouble
f_cost(toporouter_layer_t *l, GSList *closelist, GtsPoint *curpoint, GtsPoint *destpoint) {
  gdouble g = g_cost(closelist, curpoint);
  gdouble h = h_cost(l, curpoint, destpoint);
  gdouble f = g + h;

  fprintf(stderr, "fcost: point %f,%f\t\tf=%f\tg=%f\th=%f\n", curpoint->x, curpoint->y, f, g, h);
  
  return f;
}
*/
/*
GtsPoint *
lowest_fcost(toporouter_layer_t *l, GSList *list, GtsPoint *destpoint) 
{
  GtsPoint *p = GTS_POINT(list->data);
  list = list->next;
  while(list) {

    list = list->next;
  }



}
*/

#define FCOST(x) (x->gcost + x->hcost)
gdouble     
route_heap_cmp(gpointer item, gpointer data)
{
  return FCOST(TOPOROUTER_VERTEX(item));  
}
/*
gint
route_heap_cmp(gconstpointer a, gconstpointer b)
{
  ToporouterVertex *va = TOPOROUTER_VERTEX(a);
  ToporouterVertex *vb = TOPOROUTER_VERTEX(b);

  if(FCOST(va) > FCOST(vb)) return 1;
  else if(FCOST(va) < FCOST(vb)) return -1;
  return 0;
}
*/
/*
GSList *
neighbor_nodes(GtsPoint *p)
{

}
*/

#define openlist_insert(p) p->cost = f_cost(l, closelist, p, b->point);\
                                     gts_heap_insert(openlist, p)

#define closelist_insert(p) closelist = g_slist_prepend(closelist, p)
/*
void
dump_attachments(GSList *i, ToporouterVertex *curpoint, toporouter_layer_t *cur_layer)
{

  while(i) {
    toporouter_attachment_t *attachment = (toporouter_attachment_t *) i->data;
    printf("attachment: [V %f,%f] [R %f] [D %s] %s\n",
        GTS_POINT(attachment->p)->x,
        GTS_POINT(attachment->p)->y,
        attachment->r,
        (attachment->dir == CCW) ? "CCW" : "CW",
        point_visible(cur_layer, GTS_VERTEX(curpoint), GTS_VERTEX(attachment->p)) ? "(visible)" : ""
        
        );


    i = i->next;
  }


}
*/
GSList *
box_constraint_vertices(ToporouterBBox *box)
{
  GSList *i = box->constraints;
  GSList *r = NULL;

  while(i) {
    r = g_slist_insert_unique(r, GTS_SEGMENT(i->data)->v1);
    r = g_slist_insert_unique(r, GTS_SEGMENT(i->data)->v2);
    i = i->next;
  }
  return r;
}

typedef struct {
  ToporouterVertex *key;
  ToporouterVertex *result;
}toporouter_heap_search_data_t;

void 
toporouter_heap_search(gpointer data, gpointer user_data)
{
  ToporouterVertex *v = TOPOROUTER_VERTEX(data);
  toporouter_heap_search_data_t *heap_search_data = (toporouter_heap_search_data_t *)user_data;
  if(v == heap_search_data->key) heap_search_data->result = v;
}

void 
toporouter_heap_color(gpointer data, gpointer user_data)
{
  ToporouterVertex *v = TOPOROUTER_VERTEX(data);
  v->flags |= (unsigned int) user_data;
}

void
draw_route_status(toporouter_t *r, GSList *closelist, GtsEHeap *openlist, 
    ToporouterVertex *curpoint, toporouter_route_t *routedata, int count)
{
  GSList *i;
  char buffer[256];
  unsigned int mask = !(VERTEX_FLAG_RED | VERTEX_FLAG_GREEN); 

  LAYER_LOOP(PCB->Data, max_layer)
  {
    int tempnum = GetLayerNumber(PCB->Data, layer);
    i = r->layers[tempnum].vertices;
    while(i) {
      TOPOROUTER_VERTEX(i->data)->flags &= mask;
      i = i->next;
    }
  }
  END_LOOP;
  
  i = closelist;
  while(i) {
    TOPOROUTER_VERTEX(i->data)->flags |= VERTEX_FLAG_GREEN;
    i = i->next;
  }

  gts_eheap_foreach(openlist,toporouter_heap_color, (gpointer) VERTEX_FLAG_GREEN);

  while(curpoint) {
    curpoint->flags |= VERTEX_FLAG_RED;
    curpoint = curpoint->parent;
  }
  
  TOPOROUTER_VERTEX(routedata->src->point)->flags |= VERTEX_FLAG_BLUE;
  TOPOROUTER_VERTEX(routedata->dest->point)->flags |= VERTEX_FLAG_BLUE;


  LAYER_LOOP(PCB->Data, max_layer)
  {
    int tempnum = GetLayerNumber(PCB->Data, layer);
    sprintf(buffer, "viz-layer%d-%d.png", tempnum, count);
    toporouter_draw_surface(r->layers[tempnum].surface, buffer, 1280, 1280, 1, routedata);
  }
  END_LOOP;

}

GSList *
all_region_points(ToporouterVertex *p) 
{
  GSList *r = NULL, *i;
  i = p->regions;
  while(i) {
    toporouter_vertex_region_t *region = TOPOROUTER_VERTEX_REGION(i->data);
    GSList *j = region->points;
    while(j) {
      r = g_slist_insert_unique(r, j->data);
      j = j->next;
    }
    i = i->next;
  }
  
//  printf("all region points length = %d\n", g_slist_length(r));

  return r;
}

ToporouterBBox *
vertex_bbox(ToporouterVertex *v) 
{
  GSList *i = v->boxes;

  if(!i) return NULL;

  if(g_slist_length(i) > 1) {
    printf("WARNING: vertex with multiple bboxes\n");
  }

  return TOPOROUTER_BBOX(i->data);
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
vertex_net_thickness(ToporouterVertex *v) 
{
  ToporouterBBox *box = vertex_bbox(v);
  g_assert(box != NULL);
  return lookup_thickness(box->style);
}

gdouble
vertex_net_keepaway(ToporouterVertex *v) 
{
  ToporouterBBox *box = vertex_bbox(v);
  g_assert(box != NULL);
  return lookup_keepaway(box->style);
}

gdouble
edge_capacity(ToporouterEdge *e) 
{
  GtsSegment *s = GTS_SEGMENT(e);
  
  return gts_point_distance(GTS_POINT(s->v1), GTS_POINT(s->v2)) - 
    (vertex_net_thickness(TOPOROUTER_VERTEX(s->v1)) / 2.) - 
    (vertex_net_thickness(TOPOROUTER_VERTEX(s->v2)) / 2.); 

}

ToporouterEdge *
get_edge(ToporouterVertex *curpoint, ToporouterVertex *p) 
{
  GSList *i = GTS_VERTEX(curpoint)->segments;

  while(i) {
    if(TOPOROUTER_IS_EDGE(i->data)) {
      GtsSegment *s = GTS_SEGMENT(i->data);
      if((s->v1 == GTS_VERTEX(curpoint) && s->v2 == GTS_VERTEX(p)) || 
          (s->v1 == GTS_VERTEX(p) && s->v2 == GTS_VERTEX(curpoint))) {
      
        return TOPOROUTER_EDGE(s);
      }

    }
    i = i->next;
  }

  return NULL;
}

/*
 * returns region in a spanning b
 */
toporouter_vertex_region_t *
vertex_region_containing_point(ToporouterVertex *a, GtsPoint *b)
{
  GSList *i = a->regions;
  guint len = g_slist_length(a->regions);
  gdouble angle = point_xangle(GTS_POINT(a), b);

  g_assert(len > 0);

  if(len == 1) 
    return (toporouter_vertex_region_t *)a->regions->data;
  
  while(i) {
    toporouter_vertex_region_t *region = TOPOROUTER_VERTEX_REGION(i->data);
    gdouble a1,a2; 

    g_assert(region->v1 != NULL);
    g_assert(region->v2 != NULL);

    a1 = point_xangle(GTS_POINT(a), GTS_POINT(region->v1));
    a2 = point_xangle(GTS_POINT(a), GTS_POINT(region->v2));

    if((a1 >= a2 && (angle > a1 || angle < a2)) || (angle > a1 && a2 > angle))
      return region;

    i = i->next;
  }

  return NULL;
}

GSList *
segment_check_and_insert(GSList *r, ToporouterVertex *v, ToporouterVertex *dest, toporouter_route_t *data)
{
  GSList *i = g_hash_table_lookup(v->corridors, dest);
  gdouble needed_capacity = lookup_thickness(data->src->style) + (2. * lookup_keepaway(data->src->style));

  while(i) {
    if(edge_capacity(TOPOROUTER_EDGE(i->data)) < needed_capacity) 
      return r;

    i = i->next;
  }

  return g_slist_prepend(r, dest);
}

//#define TRACE_VISIBILITY TRUE

GSList *
compute_visible_points(toporouter_layer_t *l, ToporouterVertex *curpoint, toporouter_route_t *data) 
{
  GSList *r = NULL, *i;
  gdouble needed_capacity = lookup_thickness(data->src->style) + (2. * lookup_keepaway(data->src->style));

  if(curpoint->parent == NULL) {
    GSList *tempr = all_region_points(curpoint);
    i = tempr;
    while(i) {
      if(TOPOROUTER_VERTEX(i->data)->flags & VERTEX_FLAG_ATTACH)
        r = segment_check_and_insert(r, curpoint, TOPOROUTER_VERTEX(i->data), data);

      i = i->next;
    }
    
  }else if(curpoint->flags & VERTEX_FLAG_ATTACH && vertex_bbox(curpoint) == data->dest) {  
    r = g_slist_prepend(r, data->dest->point);

  }else if(curpoint->flags & VERTEX_FLAG_ATTACH && curpoint->parent == TOPOROUTER_VERTEX(data->src->point)) {
    toporouter_vertex_region_t *region = vertex_region_containing_point(curpoint, GTS_POINT(curpoint->parent));
    GSList *rr = NULL;
    g_assert(region != NULL);

    i = curpoint->regions;
    while(i) {
      toporouter_vertex_region_t *curregion = TOPOROUTER_VERTEX_REGION(i->data);
      if(curregion == region) {
        if(i->next) {
          rr = g_slist_copy(((toporouter_vertex_region_t *)i->next->data)->points);
          break;
        }
      }
      if(i->next) {
        if(((toporouter_vertex_region_t *)i->next->data) == region) {
          rr = g_slist_copy(curregion->points);
          break;
        }
      }
      i = i->next;
    }

    g_assert(rr != NULL);
    
    i = rr;
    while(i) {
      if(TOPOROUTER_VERTEX(i->data)->flags & VERTEX_FLAG_ATTACH) {
        if(vertex_bbox(TOPOROUTER_VERTEX(i->data)) == data->dest)
          r = segment_check_and_insert(r, curpoint, TOPOROUTER_VERTEX(i->data), data);
      }else
        r = segment_check_and_insert(r, curpoint, TOPOROUTER_VERTEX(i->data), data);
      i = i->next;
    }


  }else{
    gdouble a = point_xangle(GTS_POINT(curpoint), GTS_POINT(curpoint->parent));
    guint len = g_slist_length(curpoint->regions);

    g_assert(len > 0);

    if(len == 1) {
      toporouter_vertex_region_t *region = TOPOROUTER_VERTEX_REGION(curpoint->regions->data);
      r = g_slist_copy(region->points);
      goto compute_visible_points_finish;
    }
  
#ifdef TRACE_VISIBILITY 
    printf("* %d regions\n", g_slist_length(curpoint->regions));
#endif 

    i = curpoint->regions;
    while(i) {
      toporouter_vertex_region_t *region = TOPOROUTER_VERTEX_REGION(i->data);
      gdouble a1,a2; 
      gdouble parentangle, a1_p, p_a2, a1_a2;

      g_assert(region->v1 != NULL);
      g_assert(region->v2 != NULL);
      
      a1 = point_xangle(GTS_POINT(curpoint), GTS_POINT(region->v1));
      a2 = point_xangle(GTS_POINT(curpoint), GTS_POINT(region->v2));
      
#ifdef TRACE_VISIBILITY 
      printf("a1 = %f a2 = %f\n", a1, a2);
#endif 
      parentangle = point_xangle(GTS_POINT(curpoint), GTS_POINT(curpoint->parent));
      a1_p = angle_span(a1, parentangle);
      p_a2 = angle_span(parentangle, a2);
      a1_a2 = angle_span(a1, a2);
      if(a1_a2 == 0.) a1_a2 = 2. * M_PI;

#ifdef TRACE_VISIBILITY 
      printf("a1_p = %f p_a2 = %f a1_a2 = %f\n", 
          a1_p, p_a2, a1_a2);
      printf("parentangle = %f\n", parentangle);
#endif 
      if((a1 >= a2 && (a > a1 || a < a2)) || (a > a1 && a2 > a)) {

        if(p_a2 > M_PI) {
          GSList *k = region->points;
          //CW
#ifdef TRACE_VISIBILITY 
          printf(" * CW\n");
#endif 
          
#ifdef TRACE_VISIBILITY 
          printf("ZONE1:\n");
#endif 
          while(k) {
            gdouble tempangle = point_xangle(GTS_POINT(curpoint), GTS_POINT(k->data));
            ToporouterEdge *e = get_edge(curpoint, TOPOROUTER_VERTEX(k->data));
#ifdef TRACE_VISIBILITY 
            printf("p angle = %f\n", tempangle);
#endif 
            
            if(angle_span(tempangle, a2) > p_a2 - M_PI) break;
            
            if(e)
              if(edge_capacity(e) < needed_capacity) { 
#ifdef TRACE_VISIBILITY 
                printf("\tfail capacity\n");
#endif 
                g_slist_free(r);
                r = NULL;
              }
            if(TOPOROUTER_VERTEX(k->data)->flags & VERTEX_FLAG_ATTACH) {
              ToporouterBBox *vbox = vertex_bbox(TOPOROUTER_VERTEX(k->data));
              if(vbox == data->src || vbox == data->dest)
                r = segment_check_and_insert(r, curpoint, TOPOROUTER_VERTEX(k->data), data);
            }else
              r = segment_check_and_insert(r, curpoint, TOPOROUTER_VERTEX(k->data), data);
            k = k->next;
          }
#ifdef TRACE_VISIBILITY 
          printf("ZONE2:\n");
#endif 
          while(k) {
            gdouble tempangle = point_xangle(GTS_POINT(curpoint), GTS_POINT(k->data));
            ToporouterEdge *e = get_edge(curpoint, TOPOROUTER_VERTEX(k->data));
#ifdef TRACE_VISIBILITY 
            printf("p angle = %f\n", tempangle);
#endif 
            
            if(angle_span(a1, tempangle) <= a1_p) break;

            if(e) 
              if(edge_capacity(e) < needed_capacity) { 
#ifdef TRACE_VISIBILITY 
                printf("\tfail capacity\n");
#endif 
                g_slist_free(r);
                r = NULL;
                goto compute_visible_points_finish;
              }
            k = k->next;
          }

          return r;
        }else if(a1_p > M_PI) {
          //CCW
          GSList *k = region->points;
#ifdef TRACE_VISIBILITY 
          printf(" * CCW\n");
#endif 
          
#ifdef TRACE_VISIBILITY 
          printf("ZONE1:\n");
#endif 
          while(k) {
            gdouble tempangle = point_xangle(GTS_POINT(curpoint), GTS_POINT(k->data));
#ifdef TRACE_VISIBILITY 
            printf("p angle = %f\n", tempangle);
#endif 
            
            if(angle_span(tempangle, a2) > a1_a2 - a1_p) break;

            k = k->next;
          }
          
#ifdef TRACE_VISIBILITY 
          printf("ZONE2:\n");
#endif 
          while(k) {
            gdouble tempangle = point_xangle(GTS_POINT(curpoint), GTS_POINT(k->data));
            ToporouterEdge *e = get_edge(curpoint, TOPOROUTER_VERTEX(k->data));
#ifdef TRACE_VISIBILITY 
            printf("p angle = %f\n", tempangle);
#endif 
            
            if(angle_span(a1, tempangle) <= a1_p - M_PI) break;

            if(e) 
              if(edge_capacity(e) < needed_capacity) {
#ifdef TRACE_VISIBILITY 
                printf("\tfail capacity\n");
#endif 
                g_slist_free(r);
                r = NULL;
                goto compute_visible_points_finish;
              }
            k = k->next;
          }
          
#ifdef TRACE_VISIBILITY 
          printf("ZONE3:\n");
#endif 
          while(k) {
            ToporouterEdge *e = get_edge(curpoint, TOPOROUTER_VERTEX(k->data));
#ifdef TRACE_VISIBILITY 
            gdouble tempangle = point_xangle(GTS_POINT(curpoint), GTS_POINT(k->data));
            printf("p angle = %f\n", tempangle);
#endif 
            if(TOPOROUTER_VERTEX(k->data)->flags & VERTEX_FLAG_ATTACH) {
              ToporouterBBox *vbox = vertex_bbox(TOPOROUTER_VERTEX(k->data));
              if(vbox == data->src || vbox == data->dest)
                r = segment_check_and_insert(r, curpoint, TOPOROUTER_VERTEX(k->data), data);
            }else
              r = segment_check_and_insert(r, curpoint, TOPOROUTER_VERTEX(k->data), data);
            
            if(e)
              if(edge_capacity(e) < needed_capacity) { 
#ifdef TRACE_VISIBILITY 
                printf("\tfail capacity\n");
#endif 
                goto compute_visible_points_finish;
              }
            
            k = k->next;
          }
          
          goto compute_visible_points_finish;
        }
        r = NULL;
        goto compute_visible_points_finish;
      }

      i = i->next;
    }

  }

compute_visible_points_finish:

  {
    ToporouterBBox *box = vertex_bbox(curpoint);
    if(box) 
      if(vertex_bbox(curpoint)->netlist == data->src->netlist) {
        i = curpoint->zlink;
        while(i) {
          if(TOPOROUTER_VERTEX(i->data) != curpoint) 
            r = g_slist_prepend(r, i->data);
          i = i->next;
        }
      }
  }

  return r;
}

void
vertex_visibility_remove(ToporouterVertex *a, ToporouterVertex *b)
{
  GSList *i;
  
  i = a->regions;
  while(i) {
    toporouter_vertex_region_t *region = TOPOROUTER_VERTEX_REGION(i->data);

    region->points = g_slist_remove(region->points, b);

    i = i->next;
  }

}


toporouter_attachment_t *
attachment_new(ToporouterVertex *a, ToporouterVertex *b, ToporouterVertex *p, char *netlist, char *style)
{
  toporouter_attachment_t *r = malloc(sizeof(toporouter_attachment_t));
  gdouble a1 = point_xangle(GTS_POINT(p), GTS_POINT(a));        
  gdouble a2 = point_xangle(GTS_POINT(p), GTS_POINT(b));        
  
  g_assert(a != NULL);
  g_assert(b != NULL);
  g_assert(p != NULL);
  
  r->angle = MAX(angle_span(a1, a2), angle_span(a2,a1));
  r->a = a;
  r->b = b;
  r->p = p;
  r->netlist = netlist;
  r->style = style;
  return r;
}

gint
attach_cmp(gconstpointer a, gconstpointer b)
{
  toporouter_attachment_t *aa = (toporouter_attachment_t *)a;
  toporouter_attachment_t *ab = (toporouter_attachment_t *)b;
  
  if(aa->angle < ab->angle) return -1;
  if(aa->angle > ab->angle) return 1;
  return 0;
}


/* 
 * inserts b into a's visibility
 */
void
vertex_visibility_insert(ToporouterVertex *a, ToporouterVertex *b)
{
  GSList *i;
  gdouble angle = point_xangle(GTS_POINT(a), GTS_POINT(b));
  guint len = g_slist_length(a->regions);

  g_assert(len > 0);

  if(len == 1) {
    toporouter_vertex_region_t *region = TOPOROUTER_VERTEX_REGION(a->regions->data);
    if(!g_slist_find(region->points, b))
      region->points = g_slist_insert_sorted_with_data(region->points, b, point_angle_cmp, a);
    return;
  }

  i = a->regions;
  while(i) {
    toporouter_vertex_region_t *region = TOPOROUTER_VERTEX_REGION(i->data);
    gdouble a1,a2; 
   
    g_assert(region->v1 != NULL);
    g_assert(region->v2 != NULL);

    a1 = point_xangle(GTS_POINT(a), GTS_POINT(region->v1));
    a2 = point_xangle(GTS_POINT(a), GTS_POINT(region->v2));

    if((a1 >= a2 && (angle >= a1 || angle <= a2)) || (angle >= a1 && a2 >= angle)) {
      if(!g_slist_find(region->points, b))
        region->points = g_slist_insert_sorted_with_data(region->points, b, point_angle_cmp, a);
    }

    i = i->next;
  }

}

void
compute_radii(ToporouterVertex *v) 
{
  gdouble pthickness, pkeepaway, cthickness, ckeepaway;
  ToporouterBBox *bbox = vertex_bbox(v);
  GSList *i = v->attached;
  gdouble raccum = 0.;

  printf("compute_radii: \n");
  
  if(bbox->type == PIN || bbox->type == VIA) {
    PinType *pin = (PinType*)bbox->data;
    pthickness = pin->Thickness / 2.;
    pkeepaway = pin->Clearance;
  }else{
    pthickness = lookup_thickness(bbox->style) / 2.; 
    pkeepaway  = lookup_keepaway(bbox->style); 
  }

  while(i) {
    toporouter_attachment_t *attach = (toporouter_attachment_t *)i->data;
    cthickness = lookup_thickness(attach->style) / 2.; 
    ckeepaway  = lookup_keepaway(attach->style); 

    attach->r = raccum + pthickness + cthickness + MAX(pkeepaway, ckeepaway);
    raccum = attach->r;

    printf("\t[R %f] [A %f]\n", raccum, attach->angle);

    pthickness = cthickness;
    pkeepaway = ckeepaway;
    i = i->next;
  }

}

void
verify_attachments(toporouter_t *r) 
{
  int n;
  for(n=0;n<max_layer;n++) {
    GSList *i = r->layers[n].vertices;
    
    int count = 0;
    while(i) {
      if(TOPOROUTER_VERTEX(i->data)->flags & VERTEX_FLAG_ATTACH)
        count++;

      i = i->next;
    }
    printf("layer %d attach count = %d\n", n, count);
  }
}

GSList *
split_path(GSList *path) 
{
  ToporouterVertex *pv = NULL;
  GSList *curpath = NULL, *i, *paths = NULL;

  i = path;
  while(i) {
    ToporouterVertex *v = TOPOROUTER_VERTEX(i->data);
//    printf("***\n");
//    if(v) printf("v = %f,%f\n", GTS_POINT(v)->x, GTS_POINT(v)->y);
//    if(pv) printf("pv = %f,%f\n", GTS_POINT(pv)->x, GTS_POINT(pv)->y);
    
    if(pv)
    if(GTS_POINT(v)->x == GTS_POINT(pv)->x && GTS_POINT(v)->y == GTS_POINT(pv)->y) {
      paths = g_slist_prepend(paths, curpath);
      curpath = NULL;
    }
    curpath = g_slist_prepend(curpath, v);

    pv = v;
    i = i->next;
  }
  
  if(g_slist_length(curpath) > 0)
    paths = g_slist_prepend(paths, curpath);
  
  return paths;
}

GSList *
route(toporouter_t *r, toporouter_route_t *data)
{
  GtsEHeap *openlist = gts_eheap_new(route_heap_cmp, NULL);
  GSList *closelist = NULL, *paths = NULL;
  GSList *i, *j;
  int count = 0;

  toporouter_layer_t *cur_layer = &r->layers[data->src->layer];
  toporouter_layer_t *dest_layer = &r->layers[data->dest->layer];
  ToporouterVertex *curpoint = TOPOROUTER_VERTEX(data->src->point);
  ToporouterVertex *destpoint = TOPOROUTER_VERTEX(data->dest->point);

  printf(" * starting a*\n");
  data->path = NULL; 

  curpoint->parent = NULL;
  curpoint->gcost = 0.;
  curpoint->hcost = simple_h_cost(curpoint, destpoint);
  if(cur_layer != dest_layer) curpoint->hcost += r->viacost;
  gts_eheap_insert(openlist, curpoint);

  while(gts_eheap_size(openlist) > 0) {
    GSList *visible;
    data->curpoint = curpoint;
    //draw_route_status(r, closelist, openlist, curpoint, data, count++);

    curpoint = TOPOROUTER_VERTEX( gts_eheap_remove_top(openlist, NULL) );
    if(curpoint->parent && curpoint->boxes) {
      if(&r->layers[TOPOROUTER_BBOX(curpoint->boxes->data)->layer] != cur_layer) {
        cur_layer = &r->layers[TOPOROUTER_BBOX(curpoint->boxes->data)->layer];
      }
    }
    
    if(curpoint == destpoint) {
      ToporouterVertex *temppoint = curpoint;
      printf("destpoint reached\n");
      data->path = NULL;
      while(temppoint) {
        data->path = g_slist_prepend(data->path, temppoint);    
        if(temppoint->flags & VERTEX_FLAG_ATTACH) printf("contains an attach\n");
        temppoint = temppoint->parent;
      }
      goto route_finish;
    }
    closelist_insert(curpoint);

//    printf("*** COUNT = %d\n", count);
    printf(".");    
    visible = compute_visible_points(cur_layer, curpoint, data);
    /*********************
    {
      unsigned int mask = ~(VERTEX_FLAG_RED | VERTEX_FLAG_GREEN | VERTEX_FLAG_BLUE); 
      char buffer[256];

      LAYER_LOOP(PCB->Data, max_layer)
      {
        int tempnum = GetLayerNumber(PCB->Data, layer);
        i = r->layers[tempnum].vertices;
        while(i) {
          TOPOROUTER_VERTEX(i->data)->flags &= mask;
          i = i->next;
        }
      }
      END_LOOP;
      
      i = visible;
      while(i) {
        TOPOROUTER_VERTEX(i->data)->flags |= VERTEX_FLAG_GREEN;
        i = i->next;
      }
      
      curpoint->flags |= VERTEX_FLAG_BLUE;
      if(curpoint->parent) 
        curpoint->parent->flags |= VERTEX_FLAG_RED;
      LAYER_LOOP(PCB->Data, max_layer)
      {
        int tempnum = GetLayerNumber(PCB->Data, layer);
        sprintf(buffer, "viz-layer%d-%d.png", tempnum, count);
        toporouter_draw_surface(r->layers[tempnum].surface, buffer, 1280, 1280, 1, data);
      }
      END_LOOP;
    }
    *********************/
    count++;
    i = visible;
    while(i) {
      ToporouterVertex *temppoint = TOPOROUTER_VERTEX(i->data);
      if(!g_slist_find(closelist, temppoint)) {

        gdouble temp_g_cost = curpoint->gcost 
          + gts_point_distance(GTS_POINT(curpoint), GTS_POINT(temppoint));

        toporouter_heap_search_data_t heap_search_data = { temppoint, NULL };

        gts_eheap_foreach(openlist,toporouter_heap_search, &heap_search_data);

        if(heap_search_data.result) {
          if(temp_g_cost < temppoint->gcost) {
            
            temppoint->gcost = temp_g_cost;
            temppoint->parent = curpoint;
            gts_eheap_update(openlist);
          }
        }else{
          temppoint->parent = curpoint;
          temppoint->gcost = temp_g_cost;
          temppoint->hcost = simple_h_cost(temppoint, destpoint);
          if(cur_layer != dest_layer) temppoint->hcost += r->viacost;
          gts_eheap_insert(openlist, temppoint);
        }
      
      }
      i = i->next;
    }
    g_slist_free(visible);
  }

  printf("ERROR: could not find path!\n");
route_finish:
  printf(" * finished a*\n");
  LAYER_LOOP(PCB->Data, max_layer)
  {
    char buffer[256];
    int tempnum = GetLayerNumber(PCB->Data, layer);
    sprintf(buffer, "route%d.png", tempnum);
    toporouter_draw_surface(r->layers[tempnum].surface, buffer, 1280, 1280, 2, data);
  }
  END_LOOP;
 
  paths = split_path(data->path);
 
  printf("%d paths\n", g_slist_length(paths));

  /* set path stuff */
  j = paths;
  while(j) {
    ToporouterVertex *pv = NULL, *nv = NULL;
    i = (GSList *) j->data;
    while(i) {
      ToporouterVertex *v = TOPOROUTER_VERTEX(i->data);

      if(i->next) 
        nv = TOPOROUTER_VERTEX(i->next->data);
      else 
        nv = NULL;

      if(nv && pv && !(v->flags & VERTEX_FLAG_ATTACH)) {
        toporouter_attachment_t *attach = attachment_new(nv, pv, v, data->src->netlist, data->src->style);
        v->attached = g_slist_insert_sorted(v->attached, attach, attach_cmp);
        compute_radii(v);
      }

      pv = v;
      i = i->next;
    }
    j = j->next;
  }
  return paths;
}

#define TRACE_EXPORT 1

#define FARFARTV(x) ((x->next) ? ( (x->next->next) ? ((x->next->next->next) ? TOPOROUTER_VERTEX(x->next->next->next->data) : NULL ) : NULL ) : NULL)
#define FARTV(x) ((x->next) ? ( (x->next->next) ? TOPOROUTER_VERTEX(x->next->next->data) : NULL ) : NULL)
#define NEXTTV(x) ((x->next) ? TOPOROUTER_VERTEX(x->next->data) : NULL)
#define CURTV(x) TOPOROUTER_VERTEX(x->data)

#define FARPOINT(x) GTS_POINT(FARTV(x))
#define NEXTPOINT(x) GTS_POINT(NEXTTV(x))
#define CURPOINT(x) GTS_POINT(CURTV(x))

void
export_line(GtsPoint *p1, GtsPoint *p2, int layer, char *style)
{
  LineTypePtr line = CreateDrawnLineOnLayer( LAYER_PTR(layer), p1->x, p1->y, p2->x, p2->y,
      lookup_thickness(style),
      lookup_keepaway(style),
      MakeFlags (AUTOFLAG | (TEST_FLAG (CLEARNEWFLAG, PCB) ? CLEARLINEFLAG : 0)));

  AddObjectToCreateUndoList (LINE_TYPE, LAYER_PTR(layer), line, line);

}

void
export_arc_to_term(GSList *path, char *style, gdouble startangle, gdouble radius, GtsPoint *fakepoint1, GtsPoint *fakepoint2) 
{
#ifdef TRACE_EXPORT
  printf("* export_term_to_arc\n");
#endif



}

void
export_arc_to_arc(GSList *path, char *style, gdouble startangle, gdouble radius, GtsPoint *fakepoint1, GtsPoint *fakepoint2) 
{
#ifdef TRACE_EXPORT
  printf("* export_term_to_arc\n");
#endif



}

gdouble
vertex_radius(ToporouterVertex*a, ToporouterVertex *v, ToporouterVertex *b)
{
  GSList *i = v->attached;

  while(i) {
    toporouter_attachment_t *attachment = (toporouter_attachment_t *) i->data;
    if( (attachment->a == a && attachment->b == b) || (attachment->a == b && attachment->b == a) )
      return attachment->r;
    i = i->next;
  }

  if(v->flags & VERTEX_FLAG_ATTACH) {
    return (vertex_net_thickness(v) / 2.) + vertex_net_keepaway(v);

  }

  printf("WARNING: vertex_radius: segments are not attached to this vertex\n");

  return -1.;
}

#define BLANKPOINT gts_point_new(gts_point_class(), 0., 0., 0.)
#define CLOSESTPOINT(x, a, b) ((gts_point_distance(GTS_POINT(x), GTS_POINT(a)) < gts_point_distance(GTS_POINT(x), GTS_POINT(b))) ? a : b)

void
export_term_to_arc(GSList *path) 
{
  char *style = vertex_bbox(CURTV(path))->style;
#ifdef TRACE_EXPORT
  printf("* export_term_to_arc\n");
#endif

  if(NEXTTV(path)->flags & VERTEX_FLAG_ATTACH && TOPOROUTER_VERTEX(vertex_bbox(NEXTTV(path))->point) == CURTV(path)) {
    /* straight line with fakepoint */
    gdouble r = (lookup_thickness(style) / 2.) + lookup_keepaway(style);
    gdouble m = perpendicular_gradient( point_gradient(CURPOINT(path), NEXTPOINT(path)) );
    GtsPoint *temppoint[2] = { BLANKPOINT, BLANKPOINT };

    points_on_line(NEXTPOINT(path), m, r, temppoint[0], temppoint[1]);
    
    export_line(CURPOINT(path), NEXTPOINT(path), vertex_bbox(CURTV(path))->layer, style);
    
    if(FARFARTV(path)) {
      export_arc_to_arc(path->next, style, point_xangle(temppoint[0], NEXTPOINT(path)), r, temppoint[0], temppoint[1]); 
    }else{
      export_arc_to_term(path->next, style, point_xangle(temppoint[0], NEXTPOINT(path)), r, temppoint[0], temppoint[1]); 
    }

  }else{
    gdouble m = point_gradient(CURPOINT(path), NEXTPOINT(path)); 
    gdouble theta, x, y, r = vertex_radius(CURTV(path), NEXTTV(path), FARTV(path));
    gdouble tempwind;
    GtsPoint *temppoint[4] = { BLANKPOINT, BLANKPOINT, BLANKPOINT, BLANKPOINT };
    GtsPoint *startpoint;

    g_assert(r > 0);
    
    theta = acos( r / gts_point_distance(CURPOINT(path), NEXTPOINT(path)));
    x =  r * cos( theta );
    y =  r * sin( theta );
        
    points_on_line(NEXTPOINT(path), m, x, temppoint[0], temppoint[1]);
    points_on_line(CLOSESTPOINT(CURPOINT(path), temppoint[0], temppoint[1]), perpendicular_gradient(m), y, temppoint[2], temppoint[3]);

    gts_object_destroy (GTS_OBJECT (temppoint[0]));
    gts_object_destroy (GTS_OBJECT (temppoint[1]));

    tempwind = point_wind(CURPOINT(path), NEXTPOINT(path), FARPOINT(path));

    if(tempwind == 0) {
      toporouter_vertex_region_t *region1 = vertex_region_containing_point(CURTV(path), temppoint[2]);
      toporouter_vertex_region_t *region2 = vertex_region_containing_point(CURTV(path), temppoint[3]);

      if(region1 == region2) {
        /* either one constraint or none */
        if(region1->v2 == NULL) {
          /* none, so by convention go clockwise */
          if(point_wind(CURPOINT(path), temppoint[2], FARPOINT(path)) == 1) 
            startpoint = temppoint[2];
          else 
            startpoint = temppoint[3];
        }else{
          /* one, so go same direction as constraints -> next seg */
          if(point_wind(CURPOINT(path), temppoint[2], FARPOINT(path))
              == point_wind(GTS_POINT(region1->v2), NEXTPOINT(path), FARPOINT(path)))
            startpoint = temppoint[2];
          else
            startpoint = temppoint[3];
        }
      }else{
        /* pick point in region with greatest span */
        g_assert(region_span(region1) != region_span(region2));
        
        if(region_span(region1) > region_span(region2)) startpoint = temppoint[2];
        else startpoint = temppoint[3];
      }
    }else if(tempwind == point_wind(CURPOINT(path), temppoint[2], FARPOINT(path))) {
      startpoint = temppoint[2];
    }else{
      startpoint = temppoint[3];
    }
    
    export_line(CURPOINT(path), startpoint, vertex_bbox(CURTV(path))->layer, style);

    gts_object_destroy (GTS_OBJECT (temppoint[2]));
    gts_object_destroy (GTS_OBJECT (temppoint[3]));

    if(FARFARTV(path)) {
      export_arc_to_arc(path->next, style, point_xangle(NEXTPOINT(path), startpoint), r, NULL, NULL); 
    }else{
      export_arc_to_term(path->next, style, point_xangle(NEXTPOINT(path), startpoint), r, NULL, NULL); 
    }


  }

}


void
export_term_to_term(GSList *path)
{
#ifdef TRACE_EXPORT
  printf("* export_term_to_term\n");
#endif
  export_line(CURPOINT(path), NEXTPOINT(path), vertex_bbox(TOPOROUTER_VERTEX(path->data))->layer, vertex_bbox(CURTV(path))->style);
}

void
export(GSList *path) 
{
  guint pathlength = g_slist_length(path);

#ifdef TRACE_EXPORT
  printf("* export\n");
#endif

  if(pathlength < 2) return ;
  else if(pathlength == 2) export_term_to_term(path);
  else export_term_to_arc(path);

}

static int 
toporouter (int argc, char **argv, int x, int y)
{
  toporouter_t *r;
  GtsPoint *temppoint;

  r = toporouter_new();
  
//  r->flags |= TOPOROUTER_FLAG_DEBUG_CDTS;
  r->flags |= TOPOROUTER_FLAG_DUMP_CDTS;

  if((r->debug = fopen("debug.txt", "w")) == NULL) {
    printf("error opening debug output file\n");
    return 0;
  }
  printf("Toporouter! %s\n", argc > 0 ? argv[0] : "");

  if(r->flags & TOPOROUTER_FLAG_VERBOSE) {
    printf("\nParameters:\n");
    printf("\tVia max     = %d\n", r->viamax);
    printf("\tVia cost    = %f\n", r->viacost);
    printf("\tStub length = %f\n", r->stublength);
    printf("\n");
  }
  
  import_geometry(r);

  RAT_LOOP(PCB->Data);
  {
    if( TEST_FLAG(SELECTEDFLAG, line) ) {
      toporouter_route_t data;

      temppoint = gts_point_new(gts_point_class(), line->Point1.X, line->Point1.Y, 0.);
      data.src = find_routebox(r, temppoint, line->group1);
      temppoint->x = line->Point2.X;
      temppoint->y = line->Point2.Y;
      data.dest = find_routebox(r, temppoint, line->group2);

      if(data.src) dump_routebox(data.src);
      else printf("Routebox a NULL\n");
      if(data.dest) dump_routebox(data.dest);
      else printf("Routebox b NULL\n");
      
      if(data.src && data.dest) {
        GSList *paths = route(r, &data);      
        GSList *i = paths;
        while(i) {
          printf("exporting..\n");
          export((GSList *)i->data);
          g_slist_free(i->data);
          i = i->next;
        }
        g_slist_free(paths);
      }else{
        printf("ERROR: one or both routeboxes NULL\n");

      }

    }
  }
  END_LOOP;

  fclose(r->debug);
  toporouter_free(r);

  IncrementUndoSerialNumber ();
  return 0;
}

static int 
global_toporouter (int argc, char **argv, int x, int y)
{

  printf("Global Toporouter! %s\n", argc > 0 ? argv[0] : "");

  printf("sorry, global toporouter not implemented yet..\n");

  IncrementUndoSerialNumber ();

  return 0;
}

static HID_Action toporouter_action_list[] = {
  {"Toporouter", "Select a net", toporouter, "Local topological autorouter", "Toporouter()"},
  {"GlobalToporouter", NULL, global_toporouter, "Global topological autorouter", "GlobalToporouter()"}
};

REGISTER_ACTIONS (toporouter_action_list)

void hid_toporouter_init()
{
  register_toporouter_action_list();
}

