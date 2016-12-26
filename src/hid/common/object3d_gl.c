/* TODO ITEMS:
 *
 * Add PLINE simplification operation to consolidate co-circular segments for reduced geometry output.
 * Look at whether arc-* intersections can be re-constructed back to original geometry, not fall back to line-line.
 * Work on snap-rounding any edge which passes through the pixel square containing an any vertex (or intersection).
 * Avoid self-touching output in contours, where that self-touching instance creates two otherwise distinct contours or holes.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <glib.h>

#include "data.h"
#include "step_id.h"
#include "quad.h"
#include "vertex3d.h"
#include "contour3d.h"
#include "appearance.h"
#include "face3d.h"
#include "edge3d.h"
#include "object3d.h"
#include "polygon.h"
#include "rats.h"

#include "rtree.h"
#include "rotate.h"

#include "pcb-printf.h"
#include "misc.h"
#include "hid/hidint.h"

#define PERFECT_ROUND_CONTOURS
#define SUM_PINS_VIAS_ONCE
#define HASH_OBJECTS

#define REVERSED_PCB_CONTOURS 1 /* PCB Contours are reversed from the expected CCW for outer ordering - once the Y-coordinate flip is taken into account */
//#undef REVERSED_PCB_CONTOURS

#ifdef REVERSED_PCB_CONTOURS
#define COORD_TO_STEP_X(pcb, x) (COORD_TO_MM(                   (x)))
#define COORD_TO_STEP_Y(pcb, y) (COORD_TO_MM((pcb)->MaxHeight - (y)))
#define COORD_TO_STEP_Z(pcb, z) (COORD_TO_MM(                   (z)))

#define STEP_X_TO_COORD(pcb, x) (MM_TO_COORD((x)))
#define STEP_Y_TO_COORD(pcb, y) ((pcb)->MaxHeight - MM_TO_COORD((y)))
#define STEP_Z_TO_COORD(pcb, z) (MM_TO_COORD((z)))
#else
/* XXX: BROKEN UPSIDE DOWN OUTPUT */
#define COORD_TO_STEP_X(pcb, x) (COORD_TO_MM((x)))
#define COORD_TO_STEP_Y(pcb, y) (COORD_TO_MM((y)))
#define COORD_TO_STEP_Z(pcb, z) (COORD_TO_MM((z)))

#define STEP_X_TO_COORD(pcb, x) (MM_TO_COORD((x)))
#define STEP_Y_TO_COORD(pcb, y) (MM_TO_COORD((y)))
#define STEP_Z_TO_COORD(pcb, z) (MM_TO_COORD((z)))
#endif


#ifndef WIN32
/* The Linux OpenGL ABI 1.0 spec requires that we define
 * GL_GLEXT_PROTOTYPES before including gl.h or glx.h for extensions
 * in order to get prototypes:
 *   http://www.opengl.org/registry/ABI/
 */
#   define GL_GLEXT_PROTOTYPES 1
#endif

#ifdef HAVE_OPENGL_GL_H
#   include <OpenGL/gl.h>
#else
#   include <GL/gl.h>
#endif

#include "hid_draw.h"
#include "hidgl.h"
#include "face3d_gl.h"
#include "object3d_gl.h"

//static Coord board_thickness;
#define HACK_BOARD_THICKNESS board_thickness
//#define HACK_BOARD_THICKNESS MM_TO_COORD(1.6)
#define HACK_COPPER_THICKNESS MM_TO_COORD(0.035)
#define HACK_PLATED_BARREL_THICKNESS MM_TO_COORD(0.08)
#define HACK_MASK_THICKNESS MM_TO_COORD(0.01)
#define HACK_SILK_THICKNESS MM_TO_COORD(0.01)

static GList *object3d_test_objects = NULL;

static appearance *object_default_face_appearance;
static appearance *object_debug_face_appearance;
static appearance *object_selected_face_appearance;
static appearance *object_default_edge_appearance;
static appearance *object_debug_edge_appearance;
static appearance *object_selected_edge_appearance;

void
object3d_test_init (void)
{
  object3d_test_objects = object3d_from_board_outline ();

  object_default_face_appearance = make_appearance();
  appearance_set_color (object_default_face_appearance, 0.8f, 0.8f, 0.8f); /* 1.0f */
  appearance_set_alpha (object_default_face_appearance, 1.0f);

  object_debug_face_appearance = make_appearance();
  appearance_set_color (object_debug_face_appearance, 1.0f, 0.0f, 0.0f); /* 0.5f */
  appearance_set_alpha (object_debug_face_appearance, 0.5f);

  object_selected_face_appearance = make_appearance();
  appearance_set_color (object_selected_face_appearance, 0.0f, 1.0f, 1.0f); /* 0.5f */
  appearance_set_alpha (object_selected_face_appearance, 0.5f);

  object_default_edge_appearance = make_appearance();
  appearance_set_color (object_default_edge_appearance, 0.0f, 0.0f, 0.0f); /* 1.0f */
  appearance_set_alpha (object_default_edge_appearance, 1.0f);

  object_debug_edge_appearance = make_appearance();
  appearance_set_color (object_debug_edge_appearance, 1.0f, 0.0f, 0.0f); /* 1.0f */
  appearance_set_alpha (object_debug_edge_appearance, 1.0f);

  object_selected_edge_appearance = make_appearance();
  appearance_set_color (object_selected_edge_appearance, 0.0f, 1.0f, 1.0f); /* 1.0f */
  appearance_set_alpha (object_selected_edge_appearance, 1.0f);

}

float colors[12][3] = {{1., 0., 0.},
                       {1., 1., 0.},
                       {0., 1., 0.},
                       {0., 1., 1.},
                       {0.5, 0., 0.},
                       {0.5, 0.5, 0.},
                       {0., 0.5, 0.},
                       {0., 0.5, 0.5},
                       {1., 0.5, 0.5},
                       {1., 1., 0.5},
                       {0.5, 1., 0.5},
                       {0.5, 1., 1.}};


#define CIRC_SEGS_D 64.0


struct draw_info {
  hidGC gc;
  object3d *object;
  bool selected;
  bool debug_face;
};


static void
draw_linearised (edge_ref e)
{
  edge_info *info = UNDIR_DATA(e);
  double lx, ly, lz;
  double x, y, z;
  int i;

  edge_ensure_linearised (e);

  glBegin (GL_LINES);

  for (i = 0; i < info->num_linearised_vertices; i++, lx = x, ly = y, lz = z)
    {
      x = info->linearised_vertices[i * 3 + 0];
      y = info->linearised_vertices[i * 3 + 1];
      z = info->linearised_vertices[i * 3 + 2];

      if (i > 0)
        {
          glVertex3f (STEP_X_TO_COORD (PCB, lx), STEP_Y_TO_COORD (PCB, ly), STEP_Z_TO_COORD (PCB, lz));
          glVertex3f (STEP_X_TO_COORD (PCB,  x), STEP_Y_TO_COORD (PCB,  y), STEP_Z_TO_COORD (PCB,  z));
        }
    }

  glEnd ();
}

static void
draw_quad_edge (edge_ref e, void *data)
{
  edge_info *info = UNDIR_DATA(e);
  struct draw_info *d_info = data;
#if 0
  int i;

  int id = ID(e) % 12;

  glColor3f (colors[id][0], colors[id][1], colors[id][2]);
#endif
#if 0
  if (d_info->selected)
    glColor4f (0.0, 1.0, 1., 1.0);
//    glColor4f (0.0, 1.0, 1., 0.5);
  else
    glColor4f (0., 0., 0., 1.0);
//    glColor4f (1., 1., 1., 0.3);
#endif

  if (info == NULL)
    return;

  if (!d_info->selected &&
      (info->is_placeholder ||
      d_info->debug_face))
    {
//      glColor4f (1.0, 0.0, 0.0, 1.0);
      glDepthMask (TRUE);
      glDisable(GL_DEPTH_TEST);
    }

//  if (info->is_stitch)
//    return;

  draw_linearised (e);

    glEnable(GL_DEPTH_TEST);
//  glDepthMask (FALSE);
}

static void
draw_contour (contour3d *contour, void *data)
{
//  struct draw_info *info = data;
  edge_ref e;

  e = contour->first_edge;

  do
    {
      draw_quad_edge (e, data);

      /* LNEXT should take us counter-clockwise around the face */
    }
  while ((e = LNEXT(e)) != contour->first_edge);
}

static int face_no;

static void
draw_face_edges (face3d *face, void *data)
{
  struct draw_info *info = data;

  info->debug_face = (face_no == debug_integer);
//  info->debug_face = face->is_debug;

  if (face->is_debug)
    appearance_apply_gl (object_debug_edge_appearance);
  else if (info->selected)
    appearance_apply_gl (object_selected_edge_appearance);
//  else if (face->appearance)
//    appearance_apply_gl (face->apper);
//  else if (info->object)
//    appearance_apply_gl (info->object->apper);
  else
    appearance_apply_gl (object_default_edge_appearance);

  g_list_foreach (face->contours, (GFunc)draw_contour, info);

  face_no++;
}

static void
draw_face (face3d *face, void *data)
{
  struct draw_info *info = data;
  float white[] = {0.4f, 0.4f, 0.4f};
  float shininess[] = {10.0f};

  face->is_debug = (face_no == debug_integer);

  if (face->is_debug)
    appearance_apply_gl (object_debug_face_appearance);
  else if (info->selected)
    appearance_apply_gl (object_selected_face_appearance);
  else if (face->appear)
    appearance_apply_gl (face->appear);
  else if (info->object->appear)
    appearance_apply_gl (info->object->appear);
  else
    appearance_apply_gl (object_default_face_appearance);

  /* Object inherited appearances? */

  glUseProgram (0);

  glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, white);
  glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, shininess);

  face3d_fill (info->gc, face, info->selected);
//  face3d_fill (info->gc, face, (face_no == debug_integer));

//  info->debug_face = (face_no == debug_integer);
//
//  return;
//
//  if (face->contours != NULL)
//      draw_contour (face->contours->data, info);
//  printf ("Drawing face\n");
//  g_list_foreach (face->contours, (GFunc)draw_contour, info);

  face_no++;
}

void
object3d_draw (hidGC gc, object3d *object, bool selected)
{
  struct draw_info info;

//  hidglGC hidgl_gc = (hidglGC)gc;
//  hidgl_instance *hidgl = hidgl_gc->hidgl;
//  hidgl_priv *priv = hidgl->priv;

  g_return_if_fail (object->edges != NULL);

  info.gc = gc;
  info.object = object;
  info.selected = selected;

//  quad_enum ((edge_ref)object->edges->data, draw_quad_edge, NULL);
//  printf ("BEGIN DRAW...\n");
//  g_list_foreach (object->edges, (GFunc)draw_quad_edge, NULL);

//  printf ("\nDrawing object\n");

  glDisable(GL_LIGHTING); /* XXX: HACK */

  face_no = 0;
  g_list_foreach (object->faces, (GFunc)draw_face_edges, &info);

  glEnable(GL_LIGHTING); /* XXX: HACK */

  face_no = 0;
  g_list_foreach (object->faces, (GFunc)draw_face, &info);

//  printf ("....ENDED\n");
}

static void
object3d_draw_debug_single (object3d *object, void *user_data)
{
  hidGC gc = user_data;

  object3d_draw (gc, object, false);
}

void
object3d_draw_debug (hidGC gc)
{
  g_list_foreach (object3d_test_objects, (GFunc)object3d_draw_debug_single, gc);
}
