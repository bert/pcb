#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "crosshair.h"
#include "clip.h"
#include "../hidint.h"
#include "gui.h"
#include "draw.h"
#include "draw_funcs.h"
#include "rtree.h"
#include "polygon.h"
#include "gui-pinout-preview.h"
#include "pcb-printf.h"

#include "hid/common/step_id.h"
#include "hid/common/quad.h"
#include "hid/common/vertex3d.h"
#include "hid/common/contour3d.h"
#include "hid/common/appearance.h"
#include "hid/common/face3d.h"
#include "hid/common/object3d.h"

#include "hid/step/step.h" // XXX: Abstraction breaking
#include "hid/step/model.h" // XXX: Abstraction breaking
#include "hid/step/assembly.h" // XXX: Abstraction breaking

/* The Linux OpenGL ABI 1.0 spec requires that we define
 * GL_GLEXT_PROTOTYPES before including gl.h or glx.h for extensions
 * in order to get prototypes:
 *   http://www.opengl.org/registry/ABI/
 */
#define GL_GLEXT_PROTOTYPES 1

/* This follows autoconf's recommendation for the AX_CHECK_GL macro
   https://www.gnu.org/software/autoconf-archive/ax_check_gl.html */
#if defined HAVE_WINDOWS_H && defined _WIN32
#  include <windows.h>
#endif
#if defined HAVE_GL_GL_H
#  include <GL/gl.h>
#elif defined HAVE_OPENGL_GL_H
#  include <OpenGL/gl.h>
#else
#  error autoconf couldnt find gl.h
#endif

#ifdef WIN32
#   include "hid/common/glext.h"

extern PFNGLUSEPROGRAMPROC         glUseProgram;
#endif

#include <gtk/gtkgl.h>
#include "hid/common/hidgl.h"

#include "hid/common/draw_helpers.h"
#include "hid/common/trackball.h"

#include "hid/common/object3d_gl.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define STEP_TO_COORD_X(pcb, x) (  MM_TO_COORD((x)))
#define STEP_TO_COORD_Y(pcb, y) ((pcb->MaxHeight) - MM_TO_COORD((y)))
#define STEP_TO_COORD_Z(pcb, z) ( MM_TO_COORD((z)))

#define VIEW_ORTHO

extern HID ghid_hid;
extern HID_DRAW ghid_graphics;
extern HID_DRAW_CLASS ghid_graphics_class;

static hidGC current_gc = NULL;

/* Sets gport->u_gc to the "right" GC to use (wrt mask or window)
*/
#define USE_GC(gc) if (!use_gc(gc)) return

static enum mask_mode cur_mask = HID_MASK_OFF;
static GLfloat view_matrix[4][4] = {{1.0, 0.0, 0.0, 0.0},
                                    {0.0, 1.0, 0.0, 0.0},
                                    {0.0, 0.0, 1.0, 0.0},
                                    {0.0, 0.0, 0.0, 1.0}};
static GLfloat last_modelview_matrix[4][4] = {{1.0, 0.0, 0.0, 0.0},
                                              {0.0, 1.0, 0.0, 0.0},
                                              {0.0, 0.0, 1.0, 0.0},
                                              {0.0, 0.0, 0.0, 1.0}};
static GLfloat last_projection_matrix[4][4] = {{1.0, 0.0, 0.0, 0.0},
                                               {0.0, 1.0, 0.0, 0.0},
                                               {0.0, 0.0, 1.0, 0.0},
                                               {0.0, 0.0, 0.0, 1.0}};
static int global_view_2d = 1;

typedef struct render_priv {
  GdkGLConfig *glconfig;
  bool trans_lines;
  bool in_context;
  int subcomposite_stencil_bit;
  char *current_colorname;
  double current_alpha_mult;
  double current_brightness;
  double current_saturation;
  GTimer *time_since_expose;

  /* Feature for leading the user to a particular location */
  guint lead_user_timeout;
  GTimer *lead_user_timer;
  bool lead_user;
  Coord lead_user_radius;
  Coord lead_user_x;
  Coord lead_user_y;

  hidgl_instance *hidgl;
  GList *active_gc_list;
  double edit_depth;

} render_priv;

typedef struct gtk_gc_struct
{
  struct hidgl_gc_struct hidgl_gc; /* Parent */

  const char *colorname;
  double alpha_mult;
  double brightness;
  double saturation;
  Coord width;
  gint cap, join;
} *gtkGC;

static void draw_lead_user (hidGC gc, render_priv *priv);
static bool ghid_unproject_to_z_plane (int ex, int ey, Coord pcb_z, double *pcb_x, double *pcb_y);

void ghid_set_lock_effects (hidGC gc, AnyObjectType *object);

object3d *step_read_test = NULL;


/* Coordinate conversions */
/* Px converts view->pcb, Vx converts pcb->view */
static inline int
Vz (Coord z)
{
  return z / gport->view.coord_per_px + 0.5;
}

static inline Coord
Px (int x)
{
  double rv = (double)x * gport->view.coord_per_px + gport->view.x0;

  if (gport->view.flip_x)
    rv = PCB->MaxWidth - rv;

  if (rv > G_MAXINT / 4)
    rv = G_MAXINT / 4;

  if (rv < -G_MAXINT / 4)
    rv = -G_MAXINT / 4;

  return  rv;
}

static inline Coord
Py (int y)
{
  double rv = (double)y * gport->view.coord_per_px + gport->view.y0;
  if (gport->view.flip_y)
    rv = PCB->MaxHeight - rv;

  if (rv > G_MAXINT / 4)
    rv = G_MAXINT / 4;

  if (rv < -G_MAXINT / 4)
    rv = -G_MAXINT / 4;

  return  rv;
}

#define BOARD_THICKNESS         MM_TO_COORD(1.60)
#define MASK_COPPER_SPACING     MM_TO_COORD(0.05)
#define SILK_MASK_SPACING       MM_TO_COORD(0.01)
static Coord
compute_depth (int group)
{
  static int last_depth_computed = 0;

  int top_group;
  int bottom_group;
  int min_copper_group;
  int max_copper_group;
  int num_copper_groups;
  int middle_copper_group;
  int depth;

  if (global_view_2d)
    return 0;

  top_group = GetLayerGroupNumberBySide (TOP_SIDE);
  bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);

  min_copper_group = MIN (bottom_group, top_group);
  max_copper_group = MAX (bottom_group, top_group);
  num_copper_groups = max_copper_group - min_copper_group;// + 1;
//  middle_copper_group = min_copper_group + num_copper_groups / 2;
  middle_copper_group = min_copper_group;

  if (group >= 0 && group < max_group) {
    if (group >= min_copper_group && group <= max_copper_group) {
      /* XXX: IS THIS INCORRECT FOR REVERSED GROUP ORDERINGS? */
      depth = -(group - middle_copper_group) * BOARD_THICKNESS / num_copper_groups;
    } else {
      depth = 0;
    }

  } else if (SL_TYPE (group) == SL_MASK) {
    if (SL_SIDE (group) == SL_TOP_SIDE) {
      depth = -((min_copper_group - middle_copper_group) * BOARD_THICKNESS / num_copper_groups - MASK_COPPER_SPACING);
    } else {
      depth = -((max_copper_group - middle_copper_group) * BOARD_THICKNESS / num_copper_groups + MASK_COPPER_SPACING);
    }
  } else if (SL_TYPE (group) == SL_SILK) {
    if (SL_SIDE (group) == SL_TOP_SIDE) {
      depth = -((min_copper_group - middle_copper_group) * BOARD_THICKNESS / num_copper_groups - MASK_COPPER_SPACING - SILK_MASK_SPACING);
    } else {
      depth = -((max_copper_group - middle_copper_group) * BOARD_THICKNESS / num_copper_groups + MASK_COPPER_SPACING + SILK_MASK_SPACING);
    }

  } else if (SL_TYPE (group) == SL_INVISIBLE) {
    /* Same as silk, but for the back-side layer */
    if (Settings.ShowBottomSide) {
      depth = -((min_copper_group - middle_copper_group) * BOARD_THICKNESS / num_copper_groups - MASK_COPPER_SPACING - SILK_MASK_SPACING);
    } else {
      depth = -((max_copper_group - middle_copper_group) * BOARD_THICKNESS / num_copper_groups + MASK_COPPER_SPACING + SILK_MASK_SPACING);
    }
  } else if (SL_TYPE (group) == SL_RATS   ||
             SL_TYPE (group) == SL_PDRILL ||
             SL_TYPE (group) == SL_UDRILL) {
    /* Draw these at the depth we last rendered at */
    depth = last_depth_computed;
  } else if (SL_TYPE (group) == SL_PASTE  ||
             SL_TYPE (group) == SL_FAB    ||
             SL_TYPE (group) == SL_ASSY) {
    /* Layer types we don't use, which draw.c asks us about, so
     * we just return _something_ to avoid the warnign below. */
    depth = last_depth_computed;
  } else {
    /* DEFAULT CASE */
    printf ("Unknown layer group to set depth for: %i\n", group);
    depth = last_depth_computed;
  }

  last_depth_computed = depth;
  return depth;
}

static void
start_subcomposite (hidgl_instance *hidgl)
{
  render_priv *priv = gport->render_priv;
  int stencil_bit;

  /* Flush out any existing geoemtry to be rendered */
  hidgl_flush_triangles (hidgl);

  glEnable (GL_STENCIL_TEST);                                 /* Enable Stencil test */
  glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);                 /* Stencil pass => replace stencil value (with 1) */

  stencil_bit = hidgl_assign_clear_stencil_bit (hidgl);       /* Get a new (clean) bitplane to stencil with */
  glStencilMask (stencil_bit);                                /* Only write to our subcompositing stencil bitplane */
  glStencilFunc (GL_GREATER, stencil_bit, stencil_bit);       /* Pass stencil test if our assigned bit is clear */

  priv->subcomposite_stencil_bit = stencil_bit;
}

static void
end_subcomposite (hidgl_instance *hidgl)
{
  render_priv *priv = gport->render_priv;

  /* Flush out any existing geoemtry to be rendered */
  hidgl_flush_triangles (hidgl);

  hidgl_return_stencil_bit (hidgl, priv->subcomposite_stencil_bit);  /* Relinquish any bitplane we previously used */

  glStencilMask (0);
  glStencilFunc (GL_ALWAYS, 0, 0);                            /* Always pass stencil test */
  glDisable (GL_STENCIL_TEST);                                /* Disable Stencil test */

  priv->subcomposite_stencil_bit = 0;
}

static void
set_depth_on_all_active_gc (render_priv *priv, float depth)
{
  GList *iter;

  for (iter = priv->active_gc_list;
       iter != NULL;
       iter = g_list_next (iter))
    {
      hidGC gc = iter->data;

      hidgl_set_depth (gc, depth);
    }
}

/* Compute group visibility based upon on copper layers only */
static bool
is_layer_group_visible (int group)
{
  int entry;
  for (entry = 0; entry < PCB->LayerGroups.Number[group]; entry++)
    {
      int layer_idx = PCB->LayerGroups.Entries[group][entry];
      if (layer_idx >= 0 && layer_idx < max_copper_layer &&
          LAYER_PTR (layer_idx)->On)
        return true;
    }
  return false;
}

int
ghid_set_layer (const char *name, int group, int empty)
{
  render_priv *priv = gport->render_priv;
  hidgl_instance *hidgl = priv->hidgl;
  bool group_visible = false;
  bool subcomposite = true;

  if (group >= 0 && group < max_group)
    {
      priv->trans_lines = true;
      subcomposite = true;
      group_visible = is_layer_group_visible (group);
    }
  else
    {
      switch (SL_TYPE (group))
	{
	case SL_INVISIBLE:
	  priv->trans_lines = false;
	  subcomposite = false;
	  group_visible = PCB->InvisibleObjectsOn;
	  break;
	case SL_MASK:
	  priv->trans_lines = true;
	  subcomposite = false;
	  group_visible = TEST_FLAG (SHOWMASKFLAG, PCB);
	  break;
	case SL_SILK:
	  priv->trans_lines = true;
	  subcomposite = true;
	  group_visible = PCB->ElementOn;
	  break;
	case SL_ASSY:
	  break;
	case SL_PDRILL:
	case SL_UDRILL:
	  priv->trans_lines = true;
	  subcomposite = true;
	  group_visible = true;
	  break;
	case SL_RATS:
	  priv->trans_lines = true;
	  subcomposite = false;
	  group_visible = PCB->RatOn;
	  break;
	}
    }

  end_subcomposite (hidgl);

  if (group_visible && subcomposite)
    start_subcomposite (hidgl);

  /* Drawing is already flushed by {start,end}_subcomposite */
  set_depth_on_all_active_gc (priv, compute_depth (group));

  return group_visible;
}

static void
ghid_end_layer ()
{
  render_priv *priv = gport->render_priv;
  hidgl_instance *hidgl = priv->hidgl;

  end_subcomposite (hidgl);
}

void
ghid_destroy_gc (hidGC gc)
{
  render_priv *priv = gport->render_priv;

  priv->active_gc_list = g_list_remove (priv->active_gc_list, gc);

  hidgl_finish_gc (gc);
  g_free (gc);
}

hidGC
ghid_make_gc (void)
{
  render_priv *priv = gport->render_priv;
  hidGC gc = (hidGC) g_new0 (struct gtk_gc_struct, 1);
  gtkGC gtk_gc = (gtkGC)gc;

  gc->hid = &ghid_hid;
  gc->hid_draw = &ghid_graphics;

  hidgl_init_gc (priv->hidgl, gc);

  gtk_gc->colorname = Settings.BackgroundColor;
  gtk_gc->alpha_mult = 1.0;
  gtk_gc->brightness = 1.0;
  gtk_gc->saturation = 1.0;

  priv->active_gc_list = g_list_prepend (priv->active_gc_list, gc);

  return gc;
}

static void
ghid_draw_grid (hidGC gc, BoxType *drawn_area)
{
  if (Vz (PCB->Grid) < MIN_GRID_DISTANCE)
    return;

  if (gdk_color_parse (Settings.GridColor, &gport->grid_color))
    {
      gport->grid_color.red ^= gport->bg_color.red;
      gport->grid_color.green ^= gport->bg_color.green;
      gport->grid_color.blue ^= gport->bg_color.blue;
    }

  glTexCoord2f (0., 0.);

  glDisable (GL_STENCIL_TEST);
  glEnable (GL_COLOR_LOGIC_OP);
  glLogicOp (GL_XOR);

  glColor3f (gport->grid_color.red / 65535.,
             gport->grid_color.green / 65535.,
             gport->grid_color.blue / 65535.);

  hidgl_draw_grid (gc, drawn_area);

  glDisable (GL_COLOR_LOGIC_OP);
  glEnable (GL_STENCIL_TEST);
}

#if 0
/* XXX: Refactor this into hidgl common routines */
static void
load_texture_from_png (char *filename)
{
  GError *error = NULL;
  GdkPixbuf *pixbuf;
  int width;
  int height;
  int rowstride;
  /* int has_alpha; */
  int bits_per_sample;
  int n_channels;
  unsigned char *pixels;

  pixbuf = gdk_pixbuf_new_from_file (filename, &error);

  if (pixbuf == NULL) {
    g_error ("%s", error->message);
    g_error_free (error);
    error = NULL;
    return;
  }

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  /* has_alpha = gdk_pixbuf_get_has_alpha (pixbuf); */
  bits_per_sample = gdk_pixbuf_get_bits_per_sample (pixbuf);
  n_channels = gdk_pixbuf_get_n_channels (pixbuf);
  pixels = gdk_pixbuf_get_pixels (pixbuf);

  g_warn_if_fail (bits_per_sample == 8);
  g_warn_if_fail (n_channels == 4);
  g_warn_if_fail (rowstride == width * n_channels);

  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                (n_channels == 4) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, pixels);

  g_object_unref (pixbuf);
}
#endif

static void
ghid_draw_bg_image (void)
{
  static GLuint texture_handle = 0;
  GLuint current_program;

  if (!ghidgui->bg_pixbuf)
    return;

  glGetIntegerv (GL_CURRENT_PROGRAM, (GLint*)&current_program);
  glUseProgram (0);

  if (texture_handle == 0)
    {
      int width =             gdk_pixbuf_get_width (ghidgui->bg_pixbuf);
      int height =            gdk_pixbuf_get_height (ghidgui->bg_pixbuf);
      int rowstride =         gdk_pixbuf_get_rowstride (ghidgui->bg_pixbuf);
      int bits_per_sample =   gdk_pixbuf_get_bits_per_sample (ghidgui->bg_pixbuf);
      int n_channels =        gdk_pixbuf_get_n_channels (ghidgui->bg_pixbuf);
      unsigned char *pixels = gdk_pixbuf_get_pixels (ghidgui->bg_pixbuf);

      g_warn_if_fail (bits_per_sample == 8);
      g_warn_if_fail (rowstride == width * n_channels);

      glGenTextures (1, &texture_handle);
      glBindTexture (GL_TEXTURE_2D, texture_handle);

      /* XXX: We should proabbly determine what the maxmimum texture supported is,
       *      and if our image is larger, shrink it down using GDK pixbuf routines
       *      rather than having it fail below.
       */

      glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                    (n_channels == 4) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, pixels);
    }

  glBindTexture (GL_TEXTURE_2D, texture_handle);

  glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glEnable (GL_TEXTURE_2D);

  /* Render a quad with the background as a texture */

  glBegin (GL_QUADS);
  glTexCoord2d (0., 0.);
  glVertex3i (0,             0,              0);
  glTexCoord2d (1., 0.);
  glVertex3i (PCB->MaxWidth, 0,              0);
  glTexCoord2d (1., 1.);
  glVertex3i (PCB->MaxWidth, PCB->MaxHeight, 0);
  glTexCoord2d (0., 1.);
  glVertex3i (0,             PCB->MaxHeight, 0);
  glEnd ();

  glDisable (GL_TEXTURE_2D);

  glUseProgram (current_program);
}

void
ghid_use_mask (enum mask_mode mode)
{
  render_priv *priv = gport->render_priv;
  hidgl_instance *hidgl = priv->hidgl;
  static int stencil_bit = 0;

  if (mode == cur_mask)
    return;

  /* Flush out any existing geoemtry to be rendered */
  hidgl_flush_triangles (hidgl);

  switch (mode)
    {
    case HID_MASK_BEFORE:
      /* The HID asks not to receive this mask type, so warn if we get it */
      g_return_if_reached ();

    case HID_MASK_CLEAR:
      /* Write '1' to the stencil buffer where the solder-mask should not be drawn. */
      glColorMask (0, 0, 0, 0);                             /* Disable writting in color buffer */
      glDepthMask (GL_FALSE);
      glEnable (GL_STENCIL_TEST);                           /* Enable Stencil test */
      stencil_bit = hidgl_assign_clear_stencil_bit (hidgl); /* Get a new (clean) bitplane to stencil with */
      glStencilFunc (GL_ALWAYS, stencil_bit, stencil_bit);  /* Always pass stencil test, write stencil_bit */
      glStencilMask (stencil_bit);                          /* Only write to our subcompositing stencil bitplane */
      glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);           /* Stencil pass => replace stencil value (with 1) */
      break;

    case HID_MASK_AFTER:
      /* Drawing operations as masked to areas where the stencil buffer is '0' */
      glColorMask (1, 1, 1, 1);                             /* Enable drawing of r, g, b & a */
      glDepthMask (GL_TRUE);
      glStencilFunc (GL_GEQUAL, 0, stencil_bit);            /* Draw only where our bit of the stencil buffer is clear */
      glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);              /* Stencil buffer read only */
      break;

    case HID_MASK_OFF:
      /* Disable stenciling */
      hidgl_return_stencil_bit (hidgl, stencil_bit);        /* Relinquish any bitplane we previously used */
      glDisable (GL_STENCIL_TEST);                          /* Disable Stencil test */
      break;
    }
  cur_mask = mode;
}


  /* Config helper functions for when the user changes color preferences.
     |  set_special colors used in the gtkhid.
   */
static void
set_special_grid_color (void)
{
  if (!gport->colormap)
    return;
  gport->grid_color.red ^= gport->bg_color.red;
  gport->grid_color.green ^= gport->bg_color.green;
  gport->grid_color.blue ^= gport->bg_color.blue;
}

void
ghid_set_special_colors (HID_Attribute * ha)
{
  if (!ha->name || !ha->value)
    return;
  if (!strcmp (ha->name, "background-color"))
    {
      ghid_map_color_string (*(char **) ha->value, &gport->bg_color);
      set_special_grid_color ();
    }
  else if (!strcmp (ha->name, "off-limit-color"))
  {
      ghid_map_color_string (*(char **) ha->value, &gport->offlimits_color);
    }
  else if (!strcmp (ha->name, "grid-color"))
    {
      ghid_map_color_string (*(char **) ha->value, &gport->grid_color);
      set_special_grid_color ();
    }
}

typedef struct
{
  int color_set;
  GdkColor color;
  double red;
  double green;
  double blue;
} ColorCache;

static void
set_gl_color_for_gc (hidGC gc)
{
  gtkGC gtk_gc = (gtkGC)gc;
  render_priv *priv = gport->render_priv;
  static void *cache = NULL;
  hidval cval;
  ColorCache *cc;
  double r, g, b, a;
  double luminance;

  if (priv->current_colorname != NULL &&
      strcmp (priv->current_colorname, gtk_gc->colorname) == 0 &&
      priv->current_alpha_mult == gtk_gc->alpha_mult &&
      priv->current_brightness == gtk_gc->brightness &&
      priv->current_saturation == gtk_gc->saturation)
    return;

  free (priv->current_colorname);
  priv->current_colorname = NULL;

  /* If we can't set the GL colour right now, quit with
   * current_colorname set to NULL, so we don't NOOP the
   * next set_gl_color_for_gc call.
   */
  if (!priv->in_context)
    return;

  priv->current_colorname = strdup (gtk_gc->colorname);
  priv->current_alpha_mult = gtk_gc->alpha_mult;
  priv->current_brightness = gtk_gc->brightness;
  priv->current_saturation = gtk_gc->saturation;

  if (gport->colormap == NULL)
    gport->colormap = gtk_widget_get_colormap (gport->top_window);
  if (strcmp (gtk_gc->colorname, "erase") == 0)
    {
      r = gport->bg_color.red   / 65535.;
      g = gport->bg_color.green / 65535.;
      b = gport->bg_color.blue  / 65535.;
      a = 1.0;
    }
  else if (strcmp (gtk_gc->colorname, "drill") == 0)
    {
      r = gport->offlimits_color.red   / 65535.;
      g = gport->offlimits_color.green / 65535.;
      b = gport->offlimits_color.blue  / 65535.;
      a = 0.85;
    }
  else
    {
      if (hid_cache_color (0, gtk_gc->colorname, &cval, &cache))
        cc = (ColorCache *) cval.ptr;
      else
        {
          cc = (ColorCache *) malloc (sizeof (ColorCache));
          memset (cc, 0, sizeof (*cc));
          cval.ptr = cc;
          hid_cache_color (1, gtk_gc->colorname, &cval, &cache);
        }

      if (!cc->color_set)
        {
          if (gdk_color_parse (gtk_gc->colorname, &cc->color))
            gdk_color_alloc (gport->colormap, &cc->color);
          else
            gdk_color_white (gport->colormap, &cc->color);
          cc->red   = cc->color.red   / 65535.;
          cc->green = cc->color.green / 65535.;
          cc->blue  = cc->color.blue  / 65535.;
          cc->color_set = 1;
        }
      r = cc->red;
      g = cc->green;
      b = cc->blue;
      a = 0.7;
    }
  if (1) {
    double maxi, mult;
    a *= gtk_gc->alpha_mult;
    if (!priv->trans_lines)
      a = 1.0;
    maxi = r;
    if (g > maxi) maxi = g;
    if (b > maxi) maxi = b;
    mult = MIN (1 / a, 1 / maxi);
#if 1
    r = r * mult;
    g = g * mult;
    b = b * mult;
#endif
  }

  r *= gtk_gc->brightness;
  g *= gtk_gc->brightness;
  b *= gtk_gc->brightness;

  /* B/W Equivalent brightness */
  luminance = (r + g + b) / 3.0;

  /* Fade between B/W and colour */
  r = r * gtk_gc->saturation + luminance * (1.0 - gtk_gc->saturation);
  g = g * gtk_gc->saturation + luminance * (1.0 - gtk_gc->saturation);
  b = b * gtk_gc->saturation + luminance * (1.0 - gtk_gc->saturation);

  hidgl_flush_triangles (gtk_gc->hidgl_gc.hidgl);
  glColor4d (r, g, b, a);
}

void
ghid_set_color (hidGC gc, const char *name)
{
  gtkGC gtk_gc = (gtkGC)gc;

  gtk_gc->colorname = name;
  set_gl_color_for_gc (gc);
}

void
ghid_set_alpha_mult (hidGC gc, double alpha_mult)
{
  gtkGC gtk_gc = (gtkGC)gc;

  gtk_gc->alpha_mult = alpha_mult;
  set_gl_color_for_gc (gc);
}

static void
ghid_set_saturation (hidGC gc, double saturation)
{
  gtkGC gtk_gc = (gtkGC)gc;

  gtk_gc->saturation = saturation;
  set_gl_color_for_gc (gc);
}

static void
ghid_set_brightness (hidGC gc, double brightness)
{
  gtkGC gtk_gc = (gtkGC)gc;

  gtk_gc->brightness = brightness;
  set_gl_color_for_gc (gc);
}

void
ghid_set_line_cap (hidGC gc, EndCapStyle style)
{
  gtkGC gtk_gc = (gtkGC)gc;

  gtk_gc->cap = style;
}

void
ghid_set_line_width (hidGC gc, Coord width)
{
  gtkGC gtk_gc = (gtkGC)gc;

  gtk_gc->width = width;
}


void
ghid_set_draw_xor (hidGC gc, int xor)
{
  /* NOT IMPLEMENTED */

  /* Only presently called when setting up a crosshair GC.
   * We manage our own drawing model for that anyway. */
}

void
ghid_set_draw_faded (hidGC gc, int faded)
{
  printf ("ghid_set_draw_faded(%p,%d) -- not implemented\n", gc, faded);
}

void
ghid_set_line_cap_angle (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  printf ("ghid_set_line_cap_angle() -- not implemented\n");
}

static void
ghid_invalidate_current_gc (void)
{
  current_gc = NULL;
}

static int
use_gc (hidGC gc)
{
  if (gc->hid != &ghid_hid)
    {
      fprintf (stderr, "Fatal: GC from another HID passed to GTK HID\n");
      abort ();
    }

  if (current_gc == gc)
    return 1;

  current_gc = gc;

  set_gl_color_for_gc (gc);
  return 1;
}

void
ghid_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  gtkGC gtk_gc = (gtkGC)gc;
  USE_GC (gc);

  hidgl_draw_line (gc, gtk_gc->cap, gtk_gc->width, x1, y1, x2, y2, gport->view.coord_per_px);
}

void
ghid_draw_arc (hidGC gc, Coord cx, Coord cy, Coord xradius, Coord yradius,
                         Angle start_angle, Angle delta_angle)
{
  gtkGC gtk_gc = (gtkGC)gc;
  USE_GC (gc);

  hidgl_draw_arc (gc, gtk_gc->width, cx, cy, xradius, yradius,
                  start_angle, delta_angle, gport->view.coord_per_px);
}

void
ghid_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  USE_GC (gc);

  hidgl_draw_rect (gc, x1, y1, x2, y2);
}


void
ghid_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius)
{
  USE_GC (gc);

  hidgl_fill_circle (gc, cx, cy, radius);
}


void
ghid_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y)
{
  USE_GC (gc);

  hidgl_fill_polygon (gc, n_coords, x, y);
}

void
ghid_fill_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box)
{
  USE_GC (gc);

  hidgl_fill_pcb_polygon (gc, poly, clip_box);
}

void
ghid_thindraw_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box)
{
  gtkGC gtk_gc = (gtkGC)gc;

  double old_alpha_mult = gtk_gc->alpha_mult;
  common_thindraw_pcb_polygon (gc, poly, clip_box);
#if 1
  ghid_set_alpha_mult (gc, gtk_gc->alpha_mult * 0.25);
  hid_draw_fill_pcb_polygon (gc, poly, clip_box);
  ghid_set_alpha_mult (gc, old_alpha_mult);
#endif
}

void
ghid_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  USE_GC (gc);

  hidgl_fill_rect (gc, x1, y1, x2, y2);
}

void
ghid_invalidate_lr (int left, int right, int top, int bottom)
{
  ghid_invalidate_all ();
}

static gboolean
force_redraw (gpointer user_data)
{
  gdk_window_process_all_updates ();

  return G_SOURCE_REMOVE;
}

#define MAX_ELAPSED (50. / 1000.) /* 50ms */
void
ghid_invalidate_all ()
{
  render_priv *priv = gport->render_priv;
  double elapsed = g_timer_elapsed (priv->time_since_expose, NULL);

  ghid_draw_area_update (gport, NULL);

  if (elapsed > MAX_ELAPSED)
    g_idle_add_full (G_PRIORITY_HIGH, force_redraw, NULL, NULL);
}

void
ghid_notify_crosshair_change (bool changes_complete)
{
  /* We sometimes get called before the GUI is up */
  if (gport->drawing_area == NULL)
    return;

  /* FIXME: We could just invalidate the bounds of the crosshair attached objects? */
  ghid_invalidate_all ();
}

void
ghid_notify_mark_change (bool changes_complete)
{
  /* We sometimes get called before the GUI is up */
  if (gport->drawing_area == NULL)
    return;

  /* FIXME: We could just invalidate the bounds of the mark? */
  ghid_invalidate_all ();
}

static void
draw_right_cross (gint x, gint y, gint z)
{
  glVertex3i (x, 0, z);
  glVertex3i (x, PCB->MaxHeight, z);
  glVertex3i (0, y, z);
  glVertex3i (PCB->MaxWidth, y, z);
}

static void
draw_slanted_cross (gint x, gint y, gint z)
{
  gint x0, y0, x1, y1;

  x0 = x + (PCB->MaxHeight - y);
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x - y;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + (PCB->MaxWidth - x);
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - x;
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);

  x0 = x - (PCB->MaxHeight - y);
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x + y;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + x;
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - (PCB->MaxWidth - x);
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);
}

static void
draw_dozen_cross (gint x, gint y, gint z)
{
  gint x0, y0, x1, y1;
  gdouble tan60 = sqrt (3);

  x0 = x + (PCB->MaxHeight - y) / tan60;
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x - y / tan60;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + (PCB->MaxWidth - x) * tan60;
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - x * tan60;
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);

  x0 = x + (PCB->MaxHeight - y) * tan60;
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x - y * tan60;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + (PCB->MaxWidth - x) / tan60;
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - x / tan60;
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);

  x0 = x - (PCB->MaxHeight - y) / tan60;
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x + y / tan60;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + x * tan60;
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - (PCB->MaxWidth - x) * tan60;
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);

  x0 = x - (PCB->MaxHeight - y) * tan60;
  x0 = MAX(0, MIN (x0, PCB->MaxWidth));
  x1 = x + y * tan60;
  x1 = MAX(0, MIN (x1, PCB->MaxWidth));
  y0 = y + x / tan60;
  y0 = MAX(0, MIN (y0, PCB->MaxHeight));
  y1 = y - (PCB->MaxWidth - x) / tan60;
  y1 = MAX(0, MIN (y1, PCB->MaxHeight));
  glVertex3i (x0, y0, z);
  glVertex3i (x1, y1, z);
}

static void
draw_crosshair (hidGC gc, render_priv *priv)
{
  gtkGC gtk_gc = (gtkGC)gc;
  gint x, y, z;
  static int done_once = 0;
  static GdkColor cross_color;

  if (!priv->in_context)
    return;

  if (!done_once)
    {
      done_once = 1;
      /* FIXME: when CrossColor changed from config */
      ghid_map_color_string (Settings.CrossColor, &cross_color);
    }

  x = gport->crosshair_x;
  y = gport->crosshair_y;
  z = gtk_gc->hidgl_gc.depth;

  glEnable (GL_COLOR_LOGIC_OP);
  glLogicOp (GL_XOR);

  glColor3f (cross_color.red / 65535.,
             cross_color.green / 65535.,
             cross_color.blue / 65535.);

  glBegin (GL_LINES);

  draw_right_cross (x, y, z);
  if (Crosshair.shape == Union_Jack_Crosshair_Shape)
    draw_slanted_cross (x, y, z);
  if (Crosshair.shape == Dozen_Crosshair_Shape)
    draw_dozen_cross (x, y, z);

  glEnd ();

  glDisable (GL_COLOR_LOGIC_OP);
}

/* 2D, BB Via aware (with layer end-point annotations) */
static void
ghid_fill_pcb_pv_2d (hidGC fg_gc, hidGC bg_gc, PinType *pv, bool drawHole, bool mask)
{
  if (!ViaIsOnAnyVisibleLayer (pv)) /*!< XXX: Should the HID_DRAW rendering functions be aware of layer visibility? */
    {
      /* Display BB vias which are hidden due to layer selection in thindraw */
      common_thindraw_pcb_pv (Output.fgGC, Output.fgGC, pv, drawHole, mask);
    }
  else
    {
      /* First draw BB Via layer identifications if appropriate */
      if (VIA_IS_BURIED (pv) && !(TEST_FLAG (SELECTEDFLAG,  pv) ||
                                  TEST_FLAG (CONNECTEDFLAG, pv) ||
                                  TEST_FLAG (FOUNDFLAG,     pv)))
        {
          /* Need to save the colour first */
          gtkGC gtk_fg_gc = (gtkGC)fg_gc;
          char *prev_colorname = gtk_fg_gc->colorname;

          int w = (pv->Thickness - pv->DrillingHole) / 4;
          int r = pv->DrillingHole / 2 + w  / 2;
          hid_draw_set_line_cap (fg_gc, Square_Cap);
          hid_draw_set_color (fg_gc, PCB->Data->Layer[pv->BuriedFrom].Color);
          hid_draw_set_line_width (fg_gc, w);
          hid_draw_arc (fg_gc, pv->X, pv->Y, r, r, 270, 180);
          hid_draw_set_color (fg_gc, PCB->Data->Layer[pv->BuriedTo].Color);
          hid_draw_set_line_width (fg_gc, w);
          hid_draw_arc (fg_gc, pv->X, pv->Y, r, r, 90, 170);

          hid_draw_set_color (fg_gc, prev_colorname);
        }

      /* Finally draw the normal via on top*/
      common_fill_pcb_pv (fg_gc, bg_gc, pv, drawHole, mask);
    }
}

void
ghid_init_renderer (int *argc, char ***argv, GHidPort *port)
{
  render_priv *priv;
  step_model *test_model;

  port->render_priv = priv = g_new0 (render_priv, 1);

  priv->time_since_expose = g_timer_new ();

  gtk_gl_init(argc, argv);

  /* setup GL-context */
  priv->glconfig = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGBA    |
                                              GDK_GL_MODE_STENCIL |
                                              GDK_GL_MODE_DEPTH   |
                                              GDK_GL_MODE_DOUBLE);
  if (!priv->glconfig)
    {
      printf ("Could not setup GL-context!\n");
      return; /* Should we abort? */
    }

  hidgl_init ();
  priv->hidgl = hidgl_new_instance ();

  /* Setup HID function pointers specific to the GL renderer*/
  ghid_graphics_class.end_layer = ghid_end_layer;
  ghid_graphics_class.fill_pcb_polygon = ghid_fill_pcb_polygon;
  ghid_graphics_class.thindraw_pcb_polygon = ghid_thindraw_pcb_polygon;
  ghid_graphics_class.fill_pcb_pv = ghid_fill_pcb_pv_2d; /* 2D, BB Via aware (with layer end-point annotations) */

  test_model =
    NULL;
//    step_model_to_shape_master ("/home/pcjc2/gedasrc/pcb/git/src/example_step/Resistor_vr68.step");
//    step_model_to_shape_master ("/home/pcjc2/gedasrc/pcb/git/src/example_step/Ceramite_2500z_10kV.step");
//    step_model_to_shape_master ("/home/pcjc2/gedasrc/pcb/git/src/example_step/Filament_Transformer.step");
//    step_model_to_shape_master ("/home/pcjc2/gedasrc/pcb/git/src/object3d_test.step");
//    step_model_to_shape_master ("/home/pcjc2/gedasrc/pcb/git/src/step_interlayer_manual.step");
//    step_model_to_shape_master ("/home/pcjc2/gedasrc/pcb/git/src/example_step/DPAK.step");
//    step_model_to_shape_master ("/home/pcjc2/gedasrc/pcb/git/src/example_step/Inductor_R1.step");
//    step_model_to_shape_master ("/home/pcjc2/gedasrc/pcb/git/src/example_step/Capacitor_100V_10uF.step");
//    step_model_to_shape_master ("/home/pcjc2/gedasrc/pcb/git/src/example_step/shape_rep.step");
//    step_model_to_shape_master ("/home/pcjc2/gedasrc/pcb/git/src/example_step/7446722007_handfixed.stp");

  if (test_model != NULL)
    step_read_test = test_model->object;
}

void
ghid_shutdown_renderer (GHidPort *port)
{
  render_priv *priv = port->render_priv;

  hidgl_free_instance (priv->hidgl);

  ghid_cancel_lead_user ();
  g_free (port->render_priv);
  port->render_priv = NULL;
}

void
ghid_init_drawing_widget (GtkWidget *widget, GHidPort *port)
{
  render_priv *priv = port->render_priv;
  GdkGLContext *drawarea_glcontext;

  /* NB: We share with the main rendering context so we can use the
   *     same pixel shader etc..
   */
  if (widget == gport->drawing_area)
    drawarea_glcontext = NULL;
  else
    drawarea_glcontext = gtk_widget_get_gl_context (gport->drawing_area);

  gtk_widget_set_gl_capability (widget,
                                priv->glconfig,
                                drawarea_glcontext,
                                TRUE,
                                GDK_GL_RGBA_TYPE);
}

void
ghid_drawing_area_configure_hook (GHidPort *port)
{
}

gboolean
ghid_start_drawing (GHidPort *port, GtkWidget *widget)
{
  GdkGLContext *pGlContext = gtk_widget_get_gl_context (widget);
  GdkGLDrawable *pGlDrawable = gtk_widget_get_gl_drawable (widget);

  /* make GL-context "current" */
  if (!gdk_gl_drawable_gl_begin (pGlDrawable, pGlContext))
    return FALSE;

  port->render_priv->in_context = true;

  hidgl_start_render (port->render_priv->hidgl);

  return TRUE;
}

void
ghid_end_drawing (GHidPort *port, GtkWidget *widget)
{
  GdkGLDrawable *pGlDrawable = gtk_widget_get_gl_drawable (widget);

  hidgl_finish_render (port->render_priv->hidgl);

  if (gdk_gl_drawable_is_double_buffered (pGlDrawable))
    gdk_gl_drawable_swap_buffers (pGlDrawable);
  else
    glFlush ();

  port->render_priv->in_context = false;

  /* end drawing to current GL-context */
  gdk_gl_drawable_gl_end (pGlDrawable);
}

void
ghid_screen_update (void)
{
}

static int
EMark_callback (const BoxType * b, void *cl)
{
  ElementType *element = (ElementType *) b;

  DrawEMark (element, element->MarkX, element->MarkY, !FRONT (element));
  return 1;
}

static void
set_object_color (AnyObjectType *obj, char *warn_color, char *selected_color,
                  char *connected_color, char *found_color, char *normal_color)
{
  char *color;

  if      (warn_color      != NULL && TEST_FLAG (WARNFLAG,      obj)) color = warn_color;
  else if (selected_color  != NULL && TEST_FLAG (SELECTEDFLAG,  obj)) color = selected_color;
  else if (connected_color != NULL && TEST_FLAG (CONNECTEDFLAG, obj)) color = connected_color;
  else if (found_color     != NULL && TEST_FLAG (FOUNDFLAG,     obj)) color = found_color;
  else                                                                color = normal_color;

  ghid_set_lock_effects (Output.fgGC, obj);
  hid_draw_set_color (Output.fgGC, color);
}

static void
set_layer_object_color (LayerType *layer, AnyObjectType *obj)
{
  set_object_color (obj, NULL, layer->SelectedColor, PCB->ConnectedColor, PCB->FoundColor, layer->Color);
}

static void
set_pv_inlayer_color (PinType *pv, LayerType *layer, int type)
{
  ghid_set_lock_effects (Output.fgGC, (AnyObjectType *) pv);

  if (TEST_FLAG (WARNFLAG, pv))           hid_draw_set_color (Output.fgGC, PCB->WarnColor);
  else if (TEST_FLAG (SELECTEDFLAG, pv))  hid_draw_set_color (Output.fgGC, (type == VIA_TYPE) ? PCB->ViaSelectedColor
                                                                                              : PCB->PinSelectedColor);
  else if (TEST_FLAG (CONNECTEDFLAG, pv)) hid_draw_set_color (Output.fgGC, PCB->ConnectedColor);
  else if (TEST_FLAG (FOUNDFLAG, pv))     hid_draw_set_color (Output.fgGC, PCB->FoundColor);
  else
    {
      int top_group = GetLayerGroupNumberBySide (TOP_SIDE);
      int bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);
      int this_group      = GetLayerGroupNumberByPointer (layer);

      if (this_group == top_group || this_group == bottom_group)
        hid_draw_set_color (Output.fgGC, (SWAP_IDENT == (this_group == bottom_group)) ?
                                         PCB->ViaColor : PCB->InvisibleObjectsColor);
      else
        hid_draw_set_color (Output.fgGC, layer->Color);
    }
}

static void
_draw_pv_name (PinType *pv)
{
  BoxType box;
  bool vert;
  TextType text;

  if (!pv->Name || !pv->Name[0])
    text.TextString = EMPTY (pv->Number);
  else
    text.TextString = EMPTY (TEST_FLAG (SHOWNUMBERFLAG, PCB) ? pv->Number : pv->Name);

  vert = TEST_FLAG (EDGE2FLAG, pv);

  if (vert)
    {
      box.X1 = pv->X - pv->Thickness    / 2 + Settings.PinoutTextOffsetY;
      box.Y1 = pv->Y - pv->DrillingHole / 2 - Settings.PinoutTextOffsetX;
    }
  else
    {
      box.X1 = pv->X + pv->DrillingHole / 2 + Settings.PinoutTextOffsetX;
      box.Y1 = pv->Y - pv->Thickness    / 2 + Settings.PinoutTextOffsetY;
    }

  ghid_set_lock_effects (Output.fgGC, (AnyObjectType *)pv);
  hid_draw_set_color (Output.fgGC, PCB->PinNameColor);

  text.Flags = NoFlags ();
  /* Set font height to approx 56% of pin thickness */
  text.Scale = 56 * pv->Thickness / FONT_CAPHEIGHT;
  text.X = box.X1;
  text.Y = box.Y1;
  text.Direction = vert ? 1 : 0;

  hid_draw_pcb_text (Output.fgGC, &text, 0);
}

static void
_draw_pv (PinType *pv, bool draw_hole)
{
  if (TEST_FLAG (THINDRAWFLAG, PCB))
    hid_draw_thin_pcb_pv (Output.fgGC, Output.fgGC, pv, draw_hole, false);
  else
    hid_draw_fill_pcb_pv (Output.fgGC, Output.bgGC, pv, draw_hole, false);

  if (!TEST_FLAG (HOLEFLAG, pv) && TEST_FLAG (DISPLAYNAMEFLAG, pv))
    _draw_pv_name (pv);
}

static void
draw_pin (PinType *pin, bool draw_hole)
{
  set_object_color ((AnyObjectType *) pin, PCB->WarnColor, PCB->PinSelectedColor,
                    PCB->ConnectedColor, PCB->FoundColor, PCB->PinColor);

  _draw_pv (pin, draw_hole);
}

static int
pin_callback (const BoxType * b, void *cl)
{
  PinType *pin = (PinType *) b;

  if (!TEST_FLAG (HOLEFLAG, pin) && TEST_FLAG (DISPLAYNAMEFLAG, pin))
    _draw_pv_name (pin);
  draw_pin (pin, TEST_FLAG (THINDRAWFLAG, PCB));
  return 1;
}

static int
pin_name_callback (const BoxType * b, void *cl)
{
  PinType *pin = (PinType *) b;

  if (!TEST_FLAG (HOLEFLAG, pin) && TEST_FLAG (DISPLAYNAMEFLAG, pin))
    _draw_pv_name (pin);
  return 1;
}

static int
pin_inlayer_callback (const BoxType * b, void *cl)
{
  set_pv_inlayer_color ((PinType *) b, cl, PIN_TYPE);
  _draw_pv ((PinType *) b, false);
  return 1;
}

static void
draw_via (PinType *via, bool draw_hole)
{
  set_object_color ((AnyObjectType *) via, PCB->WarnColor, PCB->ViaSelectedColor,
                    PCB->ConnectedColor, PCB->FoundColor, PCB->ViaColor);

  _draw_pv (via, draw_hole);
}

typedef struct {
  int layer_group;
} via_info;


static int
via_callback (const BoxType * b, void *cl)
{
  PinType *via = (PinType *)b;

  /* NB: Called for all vias, whether BB via or not */
  draw_via (via, TEST_FLAG (THINDRAWFLAG, PCB) || !ViaIsOnAnyVisibleLayer (via));

  return 1;
}

static int
via_inlayer_callback (const BoxType * b, void *cl)
{
  PinType *via = (PinType *)b;
  LayerType *layer = (LayerType *)cl;
  int layer_group = GetLayerGroupNumberByPointer (layer);

  if (ViaIsOnLayerGroup (via, layer_group))
    {
      set_pv_inlayer_color (via, layer, VIA_TYPE);
      _draw_pv (via, TEST_FLAG (THINDRAWFLAG, PCB));
    }

  return 1;
}

static void
draw_pad_name (PadType *pad)
{
  BoxType box;
  bool vert;
  TextType text;

  if (!pad->Name || !pad->Name[0])
    text.TextString = EMPTY (pad->Number);
  else
    text.TextString = EMPTY (TEST_FLAG (SHOWNUMBERFLAG, PCB) ? pad->Number : pad->Name);

  /* should text be vertical ? */
  vert = (pad->Point1.X == pad->Point2.X);

  if (vert)
    {
      box.X1 = pad->Point1.X                      - pad->Thickness / 2;
      box.Y1 = MAX (pad->Point1.Y, pad->Point2.Y) + pad->Thickness / 2;
      box.X1 += Settings.PinoutTextOffsetY;
      box.Y1 -= Settings.PinoutTextOffsetX;
    }
  else
    {
      box.X1 = MIN (pad->Point1.X, pad->Point2.X) - pad->Thickness / 2;
      box.Y1 = pad->Point1.Y                      - pad->Thickness / 2;
      box.X1 += Settings.PinoutTextOffsetX;
      box.Y1 += Settings.PinoutTextOffsetY;
    }

  ghid_set_lock_effects (Output.fgGC, (AnyObjectType *)pad);
  hid_draw_set_color (Output.fgGC, PCB->PinNameColor);

  text.Flags = NoFlags ();
  /* Set font height to approx 90% of pad thickness */
  text.Scale = 90 * pad->Thickness / FONT_CAPHEIGHT;
  text.X = box.X1;
  text.Y = box.Y1;
  text.Direction = vert ? 1 : 0;

  hid_draw_pcb_text (Output.fgGC, &text, 0);
}

static void
_draw_pad (hidGC gc, PadType *pad, bool clear, bool mask)
{
  if (clear && !mask && pad->Clearance <= 0)
    return;

  if (TEST_FLAG (THINDRAWFLAG, PCB) ||
      (clear && TEST_FLAG (THINDRAWPOLYFLAG, PCB)))
    hid_draw_thin_pcb_pad (gc, pad, clear, mask);
  else
    hid_draw_fill_pcb_pad (gc, pad, clear, mask);
}

static void
draw_pad (PadType *pad)
{
  ghid_set_lock_effects (Output.fgGC, (AnyObjectType *)pad);
  set_object_color ((AnyObjectType *)pad, PCB->WarnColor,
                    PCB->PinSelectedColor, PCB->ConnectedColor, PCB->FoundColor,
                    FRONT (pad) ? PCB->PinColor : PCB->InvisibleObjectsColor);

  _draw_pad (Output.fgGC, pad, false, false);

  if (TEST_FLAG (DISPLAYNAMEFLAG, pad))
    draw_pad_name (pad);
}

static int
pad_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  int *side = cl;

  if (ON_SIDE (pad, *side)) {
    if (TEST_FLAG (DISPLAYNAMEFLAG, pad))
      draw_pad_name (pad);
    draw_pad (pad);
  }
  return 1;
}

static bool
via_visible_on_layer_group (PinType *via, int layer_group)
{
  if (layer_group == -1)
     return true;
  else
    return ViaIsOnLayerGroup (via, layer_group);
}

typedef struct
{
  int plated;
  bool drill_pair;
  int from_group;
  int to_group;
  int current_group;
} hole_info;

static int
hole_callback (const BoxType * b, void *cl)
{
  PinType *pv = (PinType *) b;
  hole_info default_info = {-1 /* plated = don't care */,
                            false /* drill_pair */,
                            -1 /* from_group */,
                            -1 /* to_group */,
                            -1 /* current_group */};
  hole_info *info = &default_info;

  if (cl != NULL)
    info = (hole_info *)cl;

  if (info->drill_pair)
    {
      if (info->from_group == -1 && info->to_group == -1)
        {
          if (!VIA_IS_BURIED (pv))
            goto pv_ok;
        }
      else
        {
          if (VIA_IS_BURIED (pv))
            {
              if (info->from_group == GetLayerGroupNumberByNumber (pv->BuriedFrom) &&
                  info->to_group   == GetLayerGroupNumberByNumber (pv->BuriedTo))
                goto pv_ok;
            }
        }

      return 1;
    }

pv_ok:
  if ((info->plated == 0 && !TEST_FLAG (HOLEFLAG, pv)) ||
      (info->plated == 1 &&  TEST_FLAG (HOLEFLAG, pv)))
    return 1;

  if (!via_visible_on_layer_group (pv, info->current_group))
     return 1;

  /* Don't mask out the hole if it will be "thin-drawn" due to being invisible */
  if (!ViaIsOnAnyVisibleLayer (pv))
     return 1;

  if (TEST_FLAG (THINDRAWFLAG, PCB))
    {
      if (!TEST_FLAG (HOLEFLAG, pv))
        {
          hid_draw_set_line_cap (Output.fgGC, Round_Cap);
          hid_draw_set_line_width (Output.fgGC, 0);
          hid_draw_arc (Output.fgGC, pv->X, pv->Y,
                        pv->DrillingHole / 2, pv->DrillingHole / 2, 0, 360);
        }
    }
  else
    hid_draw_fill_circle (Output.bgGC, pv->X, pv->Y, pv->DrillingHole / 2);

  if (TEST_FLAG (HOLEFLAG, pv))
    {
      ghid_set_lock_effects (Output.fgGC, (AnyObjectType *)pv);
      set_object_color ((AnyObjectType *) pv, PCB->WarnColor,
                        PCB->ViaSelectedColor, NULL, NULL, Settings.BlackColor);

      hid_draw_set_line_cap (Output.fgGC, Round_Cap);
      hid_draw_set_line_width (Output.fgGC, 0);
      hid_draw_arc (Output.fgGC, pv->X, pv->Y,
                    pv->DrillingHole / 2, pv->DrillingHole / 2, 0, 360);
    }
  return 1;
}

static int
line_callback (const BoxType * b, void *cl)
{
  LayerType *layer = cl;
  LineType *line = (LineType *)b;

  ghid_set_lock_effects (Output.fgGC, (AnyObjectType *) line);
  set_layer_object_color (layer, (AnyObjectType *) line);
  hid_draw_pcb_line (Output.fgGC, line);
  return 1;
}

static int
arc_callback (const BoxType * b, void *cl)
{
  LayerType *layer = cl;
  ArcType *arc = (ArcType *)b;

  ghid_set_lock_effects (Output.fgGC, (AnyObjectType *) arc);
  set_layer_object_color (layer, (AnyObjectType *) arc);
  hid_draw_pcb_arc (Output.fgGC, arc);
  return 1;
}

static int
text_callback (const BoxType * b, void *cl)
{
  LayerType *layer = cl;
  TextType *text = (TextType *)b;
  int min_silk_line;

  ghid_set_lock_effects (Output.fgGC, (AnyObjectType *)text);
  if (TEST_FLAG (SELECTEDFLAG, text))
    hid_draw_set_color (Output.fgGC, layer->SelectedColor);
  else
    hid_draw_set_color (Output.fgGC, layer->Color);
  if (layer == &PCB->Data->SILKLAYER ||
      layer == &PCB->Data->BACKSILKLAYER)
    min_silk_line = PCB->minSlk;
  else
    min_silk_line = PCB->minWid;
  hid_draw_pcb_text (Output.fgGC, text, min_silk_line);
  return 1;
}

struct poly_info
{
  LayerType *layer;
  const BoxType *drawn_area;
};

static int
poly_callback (const BoxType * b, void *cl)
{
  struct poly_info *i = (struct poly_info *) cl;
  PolygonType *polygon = (PolygonType *) b;

  set_layer_object_color (i->layer, (AnyObjectType *) polygon);
  hid_draw_pcb_polygon (Output.fgGC, polygon, i->drawn_area);
  return 1;
}

static int
poly_callback_no_clear (const BoxType * b, void *cl)
{
  struct poly_info *i = (struct poly_info *) cl;
  PolygonType *polygon = (PolygonType *) b;

  if (TEST_FLAG (CLEARPOLYFLAG, polygon))
    return 0;

  ghid_set_lock_effects (Output.fgGC, (AnyObjectType *) polygon);
  set_layer_object_color (i->layer, (AnyObjectType *) polygon);
  hid_draw_pcb_polygon (Output.fgGC, polygon, i->drawn_area);
  return 1;
}

static int
poly_callback_clearing (const BoxType * b, void *cl)
{
  struct poly_info *i = (struct poly_info *) cl;
  PolygonType *polygon = (PolygonType *) b;

  if (!TEST_FLAG (CLEARPOLYFLAG, polygon))
    return 0;

  ghid_set_lock_effects (Output.fgGC, (AnyObjectType *) polygon);
  set_layer_object_color (i->layer, (AnyObjectType *) polygon);
  hid_draw_pcb_polygon (Output.fgGC, polygon, i->drawn_area);
  return 1;
}

static int
clearPin_callback (const BoxType * b, void *cl)
{
  PinType *pin = (PinType *) b;
  if (TEST_FLAG (THINDRAWFLAG, PCB) || TEST_FLAG (THINDRAWPOLYFLAG, PCB))
    hid_draw_thin_pcb_pv (Output.pmGC, Output.pmGC, pin, false, true);
  else
    hid_draw_fill_pcb_pv (Output.pmGC, Output.pmGC, pin, false, true);
  return 1;
}

static int
clearPad_callback (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  int *side = cl;
  if (ON_SIDE (pad, *side) && pad->Mask)
    _draw_pad (Output.pmGC, pad, true, true);
  return 1;
}

static int
clearPin_callback_solid (const BoxType * b, void *cl)
{
  PinType *pin = (PinType *) b;
  hid_draw_fill_pcb_pv (Output.pmGC, Output.pmGC, pin, false, true);
  return 1;
}

static int
clearPad_callback_solid (const BoxType * b, void *cl)
{
  PadType *pad = (PadType *) b;
  int *side = cl;
  if (ON_SIDE (pad, *side) && pad->Mask)
    hid_draw_fill_pcb_pad (Output.pmGC, pad, true, true);
  return 1;
}

static void
ensure_board_outline (void)
{
  if (!PCB->Data->outline_valid) {

    if (PCB->Data->outline != NULL)
      poly_Free (&PCB->Data->outline);

    PCB->Data->outline = board_outline_poly (false);
    PCB->Data->outline_valid = true;
  }
}

static void
fill_board_outline (hidGC gc, const BoxType *drawn_area)
{
  PolygonType polygon;

  ensure_board_outline ();

  if (PCB->Data->outline == NULL)
    return;

  memset (&polygon, 0, sizeof (polygon));
  polygon.Clipped = PCB->Data->outline;
  if (drawn_area)
    polygon.BoundingBox = *drawn_area;
  polygon.Flags = NoFlags ();
  SET_FLAG (FULLPOLYFLAG, &polygon);
  hid_draw_fill_pcb_polygon (gc, &polygon, drawn_area);
//  hid_draw_thin_pcb_polygon (gc, &polygon, drawn_area);
  poly_FreeContours (&polygon.NoHoles);
}

struct outline_info {
  hidGC gc;
  float z1;
  float z2;
};

static int
fill_outline_hole_cb (PLINE *pl, void *user_data)
{
  struct outline_info *info = (struct outline_info *)user_data;
  PolygonType polygon;
  PLINE *pl_copy = NULL;

  poly_CopyContour (&pl_copy, pl);
  poly_InvContour (pl_copy);

  memset (&polygon, 0, sizeof (polygon));
  polygon.Clipped = poly_Create ();
  poly_InclContour (polygon.Clipped, pl_copy);

//  if (polygon.Clipped->contours == NULL)
//    return 0;

  polygon.Flags = NoFlags ();
  SET_FLAG (FULLPOLYFLAG, &polygon);

  /* XXX: For some reason, common_fill_pcb_polygon doesn't work for all contours here.. not sure why */
//  common_fill_pcb_polygon (info->gc, &polygon, NULL);
  hid_draw_fill_pcb_polygon (info->gc, &polygon, NULL);

  poly_FreeContours (&polygon.NoHoles);

  poly_Free (&polygon.Clipped);

  return 0;
}

static void
fill_board_outline_holes (hidGC gc, const BoxType *drawn_area)
{
  render_priv *priv = gport->render_priv;
  PolygonType polygon, p;
  struct outline_info info;

  ensure_board_outline ();

  memset (&polygon, 0, sizeof (polygon));
  polygon.Clipped = PCB->Data->outline;
  if (drawn_area)
    polygon.BoundingBox = *drawn_area;
  polygon.Flags = NoFlags ();
  SET_FLAG (FULLPOLYFLAG, &polygon);

  info.gc = gc;

  p = polygon;
  do {
    PolygonHoles (&p, drawn_area, fill_outline_hole_cb, &info);
  } while ((p.Clipped = p.Clipped->f) != polygon.Clipped);

//  poly_FreeContours (&polygon.NoHoles);

  hidgl_flush_triangles (priv->hidgl);
}

static void
GhidDrawMask (int side, BoxType * screen)
{
//  static bool first_run = true;
//  static GLuint texture;
  int thin = TEST_FLAG(THINDRAWFLAG, PCB) || TEST_FLAG(THINDRAWPOLYFLAG, PCB);
  LayerType *Layer = LAYER_PTR (side == TOP_SIDE ? top_soldermask_layer : bottom_soldermask_layer);
  struct poly_info info;

  OutputType *out = &Output;

  if (thin)
    {
      hid_draw_set_line_width (Output.pmGC, 0);
      hid_draw_set_color (Output.pmGC, PCB->MaskColor);
      r_search (PCB->Data->pin_tree, screen, NULL, clearPin_callback, NULL);
      r_search (PCB->Data->via_tree, screen, NULL, clearPin_callback, NULL);
      r_search (PCB->Data->pad_tree, screen, NULL, clearPad_callback, &side);
      hid_draw_set_color (Output.pmGC, "erase");
    }

  hid_draw_use_mask (&ghid_graphics, HID_MASK_CLEAR);

  info.layer = Layer;
  info.drawn_area = screen;
  r_search (Layer->polygon_tree, screen, NULL, poly_callback, &info);
  r_search (Layer->line_tree, screen, NULL, line_callback, Layer);
  r_search (Layer->arc_tree, screen, NULL, arc_callback, Layer);
  r_search (Layer->text_tree, screen, NULL, text_callback, Layer);

  r_search (PCB->Data->pin_tree, screen, NULL, clearPin_callback_solid, NULL);
  r_search (PCB->Data->via_tree, screen, NULL, clearPin_callback_solid, NULL);
  r_search (PCB->Data->pad_tree, screen, NULL, clearPad_callback_solid, &side);

  hid_draw_use_mask (&ghid_graphics, HID_MASK_AFTER);
  hid_draw_set_color (out->fgGC, PCB->MaskColor);
  ghid_set_alpha_mult (out->fgGC, thin ? 0.35 : 1.0);

#if 0
  if (first_run) {
    glGenTextures (1, &texture);
    glBindTexture (GL_TEXTURE_2D, texture);
    load_texture_from_png ("board_texture.png");
  } else {
    glBindTexture (GL_TEXTURE_2D, texture);
  }
  glUseProgram (0);

  if (1) {
    GLfloat s_params[] = {0.0001, 0., 0., 0.};
    GLfloat t_params[] = {0., 0.0001, 0., 0.};
    GLint obj_lin = GL_OBJECT_LINEAR;
    glTexGeniv (GL_S, GL_TEXTURE_GEN_MODE, &obj_lin);
    glTexGeniv (GL_T, GL_TEXTURE_GEN_MODE, &obj_lin);
    glTexGenfv (GL_S, GL_OBJECT_PLANE, s_params);
    glTexGenfv (GL_T, GL_OBJECT_PLANE, t_params);
    glEnable (GL_TEXTURE_GEN_S);
    glEnable (GL_TEXTURE_GEN_T);
  }

  glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glEnable (GL_TEXTURE_2D);
#endif

#if 0
  ensure_board_outline ();

  memset (&polygon, 0, sizeof (polygon));
  polygon.Clipped = PCB->Data->outline;
  if (screen)
    polygon.BoundingBox = *screen;
  polygon.Flags = NoFlags ();
  SET_FLAG (FULLPOLYFLAG, &polygon);
  hid_draw_fill_pcb_polygon (out->fgGC, &polygon, screen);
  poly_FreeContours (&polygon.NoHoles);
#endif

  fill_board_outline (out->fgGC, screen);

  ghid_set_alpha_mult (out->fgGC, 1.0);
//  hidgl_flush_triangles (priv->hidgl);
#if 0
  glDisable (GL_TEXTURE_GEN_S);
  glDisable (GL_TEXTURE_GEN_T);
  glBindTexture (GL_TEXTURE_2D, 0);
  glDisable (GL_TEXTURE_2D);
#endif
  hidgl_shader_activate (circular_program);

  hid_draw_use_mask (&ghid_graphics, HID_MASK_OFF);

//  first_run = false;
}

static void
draw_outline_contour (hidGC gc, PLINE *pl, float z1, float z2)
{
  VNODE *v;
  GLfloat x, y;

  hidgl_ensure_vertex_space (gc, 2 * pl->Count + 2 + 2);

  /* NB: Repeated first virtex to separate from other tri-strip */

  x = pl->head.point[0];
  y = pl->head.point[1];

  hidgl_add_vertex_3D_tex (gc, x, y, z1, 0.0, 0.0);
  hidgl_add_vertex_3D_tex (gc, x, y, z1, 0.0, 0.0);
  hidgl_add_vertex_3D_tex (gc, x, y, z2, 0.0, 0.0);

  v = pl->head.next;

  do
    {
      x = v->point[0];
      y = v->point[1];

      hidgl_add_vertex_3D_tex (gc, x, y, z1, 0.0, 0.0);
      hidgl_add_vertex_3D_tex (gc, x, y, z2, 0.0, 0.0);
    }
  while ((v = v->next) != pl->head.next);

  /* NB: Repeated last virtex to separate from other tri-strip */
  hidgl_add_vertex_3D_tex (gc, x, y, z2, 0.0, 0.0);
}

static int
outline_hole_cb (PLINE *pl, void *user_data)
{
  struct outline_info *info = (struct outline_info *)user_data;

  draw_outline_contour (info->gc, pl, info->z1, info->z2);
  return 0;
}

static void
ghid_draw_outline_between_layers (int from_layer, int to_layer, BoxType *drawn_area)
{
  render_priv *priv = gport->render_priv;
  PolygonType polygon, p;
  struct outline_info info;

  ensure_board_outline ();

  memset (&polygon, 0, sizeof (polygon));
  polygon.Clipped = PCB->Data->outline;
  if (drawn_area)
    polygon.BoundingBox = *drawn_area;
  polygon.Flags = NoFlags ();
  SET_FLAG (FULLPOLYFLAG, &polygon);

  info.gc = Output.fgGC;
  info.z1 = compute_depth (from_layer);
  info.z2 = compute_depth (to_layer);

  p = polygon;
  do {
    draw_outline_contour (info.gc, p.Clipped->contours, info.z1, info.z2);
    PolygonHoles (&p, drawn_area, outline_hole_cb, &info);
  } while ((p.Clipped = p.Clipped->f) != polygon.Clipped);

  poly_FreeContours (&polygon.NoHoles);

  hidgl_flush_triangles (priv->hidgl);
}

static int
GhidDrawLayerGroup (int group, const BoxType * screen)
{
  render_priv *priv = gport->render_priv;
  int i;
  int layernum;
  int side;
  struct poly_info info;
  LayerType *Layer;
  int n_entries = PCB->LayerGroups.Number[group];
  Cardinal *layers = PCB->LayerGroups.Entries[group];
  int first_run = 1;
  int top_group = GetLayerGroupNumberBySide (TOP_SIDE);
  int bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);
  hole_info h_info = {-1 /* plated = don't care */,
                      false /* drill_pair */,
                      -1 /* from_group */,
                      -1 /* to_group */,
                      group /* current_group */};
  bool is_outline;

  if (!hid_draw_set_layer (&ghid_graphics, 0, group, 0))
    return 0;

  /* HACK: Subcomposite each layer in a layer group separately */
  for (i = n_entries - 1; i >= 0; i--) {
    layernum = layers[i];
    Layer = PCB->Data->Layer + layers[i];

    is_outline = strcmp (Layer->Name, "outline") == 0 ||
                 strcmp (Layer->Name, "route") == 0;

    if (layernum < max_copper_layer && Layer->On) {

      if (!first_run)
        hid_draw_set_layer (&ghid_graphics, 0, group, 0);

      first_run = 0;

      if (!is_outline && !TEST_FLAG (THINDRAWFLAG, PCB)) {
        /* Mask out drilled holes on this layer */
        hidgl_flush_triangles (priv->hidgl);
        glPushAttrib (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glColorMask (0, 0, 0, 0);
        glDepthMask (GL_FALSE);
        hid_draw_set_color (Output.bgGC, PCB->MaskColor);
        if (PCB->PinOn) r_search (PCB->Data->pin_tree, screen, NULL, hole_callback, NULL);
        if (PCB->ViaOn) r_search (PCB->Data->via_tree, screen, NULL, hole_callback, &h_info);
        fill_board_outline_holes (Output.bgGC, screen);
        hidgl_flush_triangles (priv->hidgl);
        glPopAttrib ();
      }

      /* draw all polygons on this layer */
      if (Layer->PolygonN) {
        info.layer = Layer;
        info.drawn_area = screen;
        r_search (Layer->polygon_tree, screen, NULL, poly_callback_no_clear, &info);
        r_search (Layer->polygon_tree, screen, NULL, poly_callback_clearing, &info);

        /* HACK: Subcomposite polygons separately from other layer primitives */
        /* Reset the compositing */
        hid_draw_end_layer (&ghid_graphics);
        hid_draw_set_layer (&ghid_graphics, 0, group, 0);

        if (!is_outline && !TEST_FLAG (THINDRAWFLAG, PCB)) {
          hidgl_flush_triangles (priv->hidgl);
          glPushAttrib (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
          glColorMask (0, 0, 0, 0);
          glDepthMask (GL_FALSE);
          /* Mask out drilled holes on this layer */
          if (PCB->PinOn) r_search (PCB->Data->pin_tree, screen, NULL, hole_callback, NULL);
          if (PCB->ViaOn) r_search (PCB->Data->via_tree, screen, NULL, hole_callback, &h_info);
          fill_board_outline_holes (Output.bgGC, screen);
          hidgl_flush_triangles (priv->hidgl);
          glPopAttrib ();
        }
      }

      /* Draw pins, vias and pads on this layer */
      if (!global_view_2d && !is_outline) {
        if (PCB->PinOn &&
            (group == bottom_group || group == top_group))
          r_search (PCB->Data->pin_tree, screen, NULL, pin_name_callback, Layer);
        if (PCB->PinOn) r_search (PCB->Data->pin_tree, screen, NULL, pin_inlayer_callback, Layer);
        if (PCB->ViaOn) r_search (PCB->Data->via_tree, screen, NULL, via_inlayer_callback, Layer);
        if (PCB->PinOn && group == top_group)
          {
            side = TOP_SIDE;
            r_search (PCB->Data->pad_tree, screen, NULL, pad_callback, &side);
          }
        if (PCB->PinOn && group == bottom_group)
          {
            side = BOTTOM_SIDE;
            r_search (PCB->Data->pad_tree, screen, NULL, pad_callback, &side);
          }
      }

      if (TEST_FLAG (CHECKPLANESFLAG, PCB))
        continue;

      r_search (Layer->line_tree, screen, NULL, line_callback, Layer);
      r_search (Layer->arc_tree, screen, NULL, arc_callback, Layer);
      r_search (Layer->text_tree, screen, NULL, text_callback, Layer);
    }
  }

  hid_draw_end_layer (&ghid_graphics);

  return (n_entries > 1);
}

static void
DrawDrillChannel (hidGC gc, int vx, int vy, int vr, int from_layer, int to_layer, double scale)
{
#define PIXELS_PER_CIRCLINE 5.
#define MIN_FACES_PER_CYL 6
#define MAX_FACES_PER_CYL 360
  float radius = vr;
  float x, y, z1, z2;
  int i;
  int slices;

  slices = M_PI * 2 * vr / scale / PIXELS_PER_CIRCLINE;

  if (slices < MIN_FACES_PER_CYL)
    slices = MIN_FACES_PER_CYL;

  if (slices > MAX_FACES_PER_CYL)
    slices = MAX_FACES_PER_CYL;

  z1 = compute_depth (from_layer);
  z2 = compute_depth (to_layer);

  x = vx + vr;
  y = vy;

  hidgl_ensure_vertex_space (gc, 2 * slices + 2 + 2);

  /* NB: Repeated first virtex to separate from other tri-strip */
  hidgl_add_vertex_3D_tex (gc, x, y, z1, 0.0, 0.0);
  hidgl_add_vertex_3D_tex (gc, x, y, z1, 0.0, 0.0);
  hidgl_add_vertex_3D_tex (gc, x, y, z2, 0.0, 0.0);

  for (i = 0; i < slices; i++)
    {
      x = radius * cosf (((float)(i + 1)) * 2. * M_PI / (float)slices) + vx;
      y = radius * sinf (((float)(i + 1)) * 2. * M_PI / (float)slices) + vy;

      hidgl_add_vertex_3D_tex (gc, x, y, z1, 0.0, 0.0);
      hidgl_add_vertex_3D_tex (gc, x, y, z2, 0.0, 0.0);
    }

  /* NB: Repeated last virtex to separate from other tri-strip */
  hidgl_add_vertex_3D_tex (gc, x, y, z2, 0.0, 0.0);
}

struct cyl_info {
  int from_layer_group;
  int to_layer_group;
  double scale;
};

static int
draw_hole_cyl (PinType *Pin, struct cyl_info *info, int Type)
{
  char *color;

  if (TEST_FLAG (WARNFLAG, Pin))
    color = PCB->WarnColor;
  else if (TEST_FLAG (SELECTEDFLAG, Pin))
    color = (Type == VIA_TYPE) ? PCB->ViaSelectedColor : PCB->PinSelectedColor;
  else if (TEST_FLAG (CONNECTEDFLAG, Pin))
    color = PCB->ConnectedColor;
  else if (TEST_FLAG (FOUNDFLAG, Pin))
    color = PCB->FoundColor;
  else
    color = "drill";

  ghid_set_lock_effects (Output.fgGC, (AnyObjectType *)Pin);
  hid_draw_set_color (Output.fgGC, color);
  DrawDrillChannel (Output.fgGC, Pin->X, Pin->Y, Pin->DrillingHole / 2, info->from_layer_group, info->to_layer_group, info->scale);
  return 0;
}

static int
pin_hole_cyl_callback (const BoxType * b, void *cl)
{
  return draw_hole_cyl ((PinType *)b, (struct cyl_info *)cl, PIN_TYPE);
}

static int
via_hole_cyl_callback (const BoxType * b, void *cl)
{
  PinType *via = (PinType *)b;
  struct cyl_info *info = (struct cyl_info *)cl;

  if (ViaIsOnLayerGroup (via, info->from_layer_group) &&
      ViaIsOnLayerGroup (via, info->to_layer_group))
    draw_hole_cyl ((PinType *)b, info, VIA_TYPE);

  return 0;
}

static void
hidgl_draw_step_model_instance (struct assembly_model_instance *instance, bool selected)
{
  render_priv *priv = gport->render_priv;
  step_model *step_model = instance->model->step_model;
  GLfloat m[4][4];
  double ox, oy, oz;

  if (step_model == NULL)
    return;

  hidgl_flush_triangles (priv->hidgl);
  glPushAttrib (GL_CURRENT_BIT);
  glPushMatrix ();

  glColor4f (1.0f, 1.0f, 1.0f, 1.0f);

//  /* KLUDGE */
//  glTranslatef (0.0, 0.0, BOARD_THICKNESS / 2.0);


  // OpenGL matrix layout (numbers are memory offsets)
  // [ 0  4   8  12 ]
  // [ 1  5   9  13 ]
  // [ 2  6  10  14 ]
  // [ 3  7  11  15 ]

  glTranslatef (STEP_TO_COORD_X (PCB, instance->ox),
                STEP_TO_COORD_Y (PCB, instance->oy),
                STEP_TO_COORD_Z (PCB, instance->oz));

  /* Undo -Y coord scaling */
  glScalef (1.0f, -1.0f, 1.0f);

  ox = instance->ay * instance->rz - instance->az * instance->ry;
  oy = instance->az * instance->rx - instance->ax * instance->rz;
  oz = instance->ax * instance->ry - instance->ay * instance->rx;
  m[0][0] = instance->rx;  m[1][0] = ox;    m[2][0] = instance->ax;    m[3][0] = 0.0f;
  m[0][1] = instance->ry;  m[1][1] = oy;    m[2][1] = instance->ay;    m[3][1] = 0.0f;
  m[0][2] = instance->rz;  m[1][2] = oz;    m[2][2] = instance->az;    m[3][2] = 0.0f;
  m[0][3] = 0.0f;          m[1][3] = 0.0f;  m[2][3] = 0.0f;            m[3][3] = 1.0f;
  glMultMatrixf(&m[0][0]);

  ox = step_model->ay * step_model->rz - step_model->az * step_model->ry;
  oy = step_model->az * step_model->rx - step_model->ax * step_model->rz;
  oz = step_model->ax * step_model->ry - step_model->ay * step_model->rx;
  // NB: The matrix indexes below are transposed from the visual layout
  // As the matrix is orthogonal, this should give us the inverse matrix
  m[0][0] = step_model->rx;  m[0][1] = ox;    m[0][2] = step_model->ax;  m[0][3] = 0.0f;
  m[1][0] = step_model->ry;  m[1][1] = oy;    m[1][2] = step_model->ay;  m[1][3] = 0.0f;
  m[2][0] = step_model->rz;  m[2][1] = oz;    m[2][2] = step_model->az;  m[2][3] = 0.0f;
  m[3][0] = 0.0f;            m[3][1] = 0.0f;  m[3][2] = 0.0f;            m[3][3] = 1.0f;
  glMultMatrixf(&m[0][0]);

  /* Undo -Y coord scaling */
  glScalef (1.0f, -1.0f, 1.0f);

  glTranslatef (-STEP_TO_COORD_X (PCB, step_model->ox),
                -STEP_TO_COORD_Y (PCB, step_model->oy),
                -STEP_TO_COORD_Z (PCB, step_model->oz));

  object3d_draw (Output.fgGC, step_model->object, selected);

  hidgl_flush_triangles (priv->hidgl);

  glPopMatrix ();
  glPopAttrib ();
}

static int
E_package_callback (const BoxType * b, void *cl)
{
  ElementType *element = (ElementType *) b;
  int layer_group = FRONT (element) ? 0 : max_copper_layer - 1; /* XXX: FIXME */
  Coord depth = compute_depth (layer_group);


  if (element->assembly_model_instance != NULL)
    {
      hidgl_draw_step_model_instance (element->assembly_model_instance, TEST_FLAG (SELECTEDFLAG, element));
    }

  if (FRONT (element))
    {
      if (element->Name[DESCRIPTION_INDEX].TextString == NULL)
        return 0;

      if (strcmp (element->Name[DESCRIPTION_INDEX].TextString, "ACY400") == 0) {
        hidgl_draw_acy_resistor (element, depth, BOARD_THICKNESS);
      }

      if (strcmp (element->Name[DESCRIPTION_INDEX].TextString, "800mil_resistor.fp") == 0) {
        hidgl_draw_800mil_resistor (element, depth, BOARD_THICKNESS);
      }

      if (strcmp (element->Name[DESCRIPTION_INDEX].TextString, "2300mil_resistor.fp") == 0) {
        hidgl_draw_2300mil_resistor (element, depth, BOARD_THICKNESS);
      }

      if (strcmp (element->Name[DESCRIPTION_INDEX].TextString, "diode_700mil_surface.fp") == 0) {
        hidgl_draw_700mil_diode_smd (element, depth, BOARD_THICKNESS);
      }

      if (strcmp (element->Name[DESCRIPTION_INDEX].TextString, "cap_100V_10uF.fp") == 0) {
        hidgl_draw_1650mil_cap (element, depth, BOARD_THICKNESS);
      }

      if (strcmp (element->Name[DESCRIPTION_INDEX].TextString, "cap_15000V_2500pF.fp") == 0) {
        hidgl_draw_350x800mil_cap (element, depth, BOARD_THICKNESS);
      }
    }
  return 1;
}

static void
ghid_draw_packages (BoxType *drawn_area)
{
  /* XXX: 3D model may be on-screen, even if drawn_area doesn't include its projection on the board */
//  r_search (PCB->Data->element_tree, drawn_area, NULL, E_package_callback, NULL);
  r_search (PCB->Data->element_tree, NULL, NULL, E_package_callback, NULL);
}

void
ghid_draw_everything (BoxType *drawn_area)
{
  render_priv *priv = gport->render_priv;
  int i, ngroups;
  int number_phys_on_top;
  int side;
  /* This is the list of layer groups we will draw.  */
  int do_group[MAX_LAYER];
  /* This is the reverse of the order in which we draw them.  */
  int drawn_groups[MAX_LAYER];
  struct cyl_info cyl_info;
  int reverse_layers;
  int save_show_solder;
  int top_group;
  int bottom_group;
  int min_phys_group;
  int max_phys_group;

  priv->current_colorname = NULL;

  /* Test direction of rendering */
  /* Look at sign of eye coordinate system z-coord when projecting a
     world vector along +ve Z axis, (0, 0, 1). */
  /* XXX: This isn't strictly correct, as I've ignored the matrix
     elements for homogeneous coordinates. */
  /* NB: last_modelview_matrix is transposed in memory! */
  reverse_layers = (last_modelview_matrix[2][2] < 0);

  save_show_solder = Settings.ShowBottomSide;
  Settings.ShowBottomSide = reverse_layers;

  PCB->Data->SILKLAYER.Color = PCB->ElementColor;
  PCB->Data->BACKSILKLAYER.Color = PCB->InvisibleObjectsColor;

  top_group = GetLayerGroupNumberBySide (TOP_SIDE);
  bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);

  min_phys_group = MIN (bottom_group, top_group);
  max_phys_group = MAX (bottom_group, top_group);

  memset (do_group, 0, sizeof (do_group));
  if (global_view_2d) {
    /* Draw in layer stack order when in 2D view */
    for (ngroups = 0, i = 0; i < max_copper_layer; i++) {
      int group = GetLayerGroupNumberByNumber (LayerStack[i]);

      if (!do_group[group]) {
        do_group[group] = 1;
        drawn_groups[ngroups++] = group;
      }
    }
  } else {
    /* Draw in group order when in 3D view */
    for (ngroups = 0, i = 0; i < max_group; i++) {
      int group = reverse_layers ? max_group - 1 - i : i;

      if (!do_group[group]) {
        do_group[group] = 1;
        drawn_groups[ngroups++] = group;
      }
    }
  }

  /*
   * first draw all 'invisible' stuff
   */
  side = SWAP_IDENT ? TOP_SIDE : BOTTOM_SIDE;

  if (!TEST_FLAG (CHECKPLANESFLAG, PCB) &&
      hid_draw_set_layer (&ghid_graphics, "invisible", SL (INVISIBLE, 0), 0)) {
    DrawSilk (&ghid_graphics, side, drawn_area);

    if (global_view_2d)
      r_search (PCB->Data->pad_tree, drawn_area, NULL, pad_callback, &side);

    hid_draw_end_layer (&ghid_graphics);

    /* Draw the reverse-side solder mask if turned on */
    if (!global_view_2d &&
        hid_draw_set_layer (&ghid_graphics, SWAP_IDENT ? "componentmask" : "soldermask",
                        SWAP_IDENT ? SL (MASK, TOP) : SL (MASK, BOTTOM), 0)) {
        GhidDrawMask (side, drawn_area);
        hid_draw_end_layer (&ghid_graphics);
      }
  }

  /* draw all layers in layerstack order */
#define FADE_FACTOR 1
  number_phys_on_top = max_phys_group - min_phys_group;
  for (i = ngroups - 1; i >= 0; i--) {
    bool is_this_physical = drawn_groups[i] >= min_phys_group &&
                            drawn_groups[i] <= max_phys_group;
    bool is_next_physical = i > 0 &&
                            drawn_groups[i - 1] >= min_phys_group &&
                            drawn_groups[i - 1] <= max_phys_group;

    double alpha_mult = global_view_2d ? pow (FADE_FACTOR, i) :
      (is_this_physical ? pow (FADE_FACTOR, number_phys_on_top) : 1.);

    if (is_this_physical)
      number_phys_on_top --;

    ghid_set_alpha_mult (Output.fgGC, alpha_mult);
    GhidDrawLayerGroup (drawn_groups [i], drawn_area);

#if 1
    if (!global_view_2d && is_this_physical && is_next_physical) {
      glDepthMask (FALSE); // Temporary kludge - lets us see objects through the sides of the board
      cyl_info.from_layer_group = drawn_groups[i];
      cyl_info.to_layer_group = drawn_groups[i - 1];
      cyl_info.scale = gport->view.coord_per_px;
      hid_draw_set_color (Output.fgGC, "drill");
      ghid_set_alpha_mult (Output.fgGC, alpha_mult * 0.75);
      ghid_draw_outline_between_layers (cyl_info.from_layer_group, cyl_info.to_layer_group, drawn_area);
      if (PCB->PinOn) r_search (PCB->Data->pin_tree, drawn_area, NULL, pin_hole_cyl_callback, &cyl_info);
      if (PCB->ViaOn) r_search (PCB->Data->via_tree, drawn_area, NULL, via_hole_cyl_callback, &cyl_info);
      glDepthMask (TRUE); // Temporary kludge - lets us see objects through the sides of the board
    }
#endif
  }
#undef FADE_FACTOR

  ghid_set_alpha_mult (Output.fgGC, 1.0);

  if (TEST_FLAG (CHECKPLANESFLAG, PCB))
    return;

  side = SWAP_IDENT ? BOTTOM_SIDE : TOP_SIDE;

  /* Draw pins, pads, vias below silk */
  if (global_view_2d) {

    start_subcomposite (priv->hidgl);

    if (!TEST_FLAG (THINDRAWFLAG, PCB)) {
      /* Mask out drilled holes */
      hidgl_flush_triangles (priv->hidgl);
      glPushAttrib (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glColorMask (0, 0, 0, 0);
      glDepthMask (GL_FALSE);
      if (PCB->PinOn) r_search (PCB->Data->pin_tree, drawn_area, NULL, hole_callback, NULL);
      if (PCB->ViaOn) r_search (PCB->Data->via_tree, drawn_area, NULL, hole_callback, NULL);
      fill_board_outline_holes (Output.bgGC, drawn_area);
      hidgl_flush_triangles (priv->hidgl);
      glPopAttrib ();
    }

    if (PCB->PinOn) r_search (PCB->Data->pad_tree, drawn_area, NULL, pad_callback, &side);
    if (PCB->PinOn) r_search (PCB->Data->pin_tree, drawn_area, NULL, pin_callback, NULL);
    if (PCB->ViaOn) r_search (PCB->Data->via_tree, drawn_area, NULL, via_callback, NULL);

    end_subcomposite (priv->hidgl);
  }

  /* Draw the solder mask if turned on */
  if (hid_draw_set_layer (&ghid_graphics, SWAP_IDENT ? "soldermask" : "componentmask",
                      SWAP_IDENT ? SL (MASK, BOTTOM) : SL (MASK, TOP), 0)) {
    GhidDrawMask (side, drawn_area);
    hid_draw_end_layer (&ghid_graphics);
  }

  if (hid_draw_set_layer (&ghid_graphics, SWAP_IDENT ? "bottomsilk" : "topsilk",
                      SWAP_IDENT ? SL (SILK, BOTTOM) : SL (SILK, TOP), 0)) {
      DrawSilk (&ghid_graphics, side, drawn_area);
      hid_draw_end_layer (&ghid_graphics);
  }

  /* Draw element Marks */
  if (PCB->PinOn)
    r_search (PCB->Data->element_tree, drawn_area, NULL, EMark_callback, NULL);

  /* Draw rat lines on top */
  if (PCB->RatOn && hid_draw_set_layer (&ghid_graphics, "rats", SL (RATS, 0), 0)) {
    DrawRats (&ghid_graphics, drawn_area);
    hid_draw_end_layer (&ghid_graphics);
  }

  Settings.ShowBottomSide = save_show_solder;
}

#define Z_NEAR 3.0
gboolean
ghid_drawing_area_expose_cb (GtkWidget *widget,
                             GdkEventExpose *ev,
                             GHidPort *port)
{
  render_priv *priv = port->render_priv;
  GtkAllocation allocation;
  BoxType region;
  double min_x, min_y;
  double max_x, max_y;
  double new_x, new_y;
  Coord min_depth;
  Coord max_depth;
  float aspect;
  GLfloat scale[] = {1, 0, 0, 0,
                     0, 1, 0, 0,
                     0, 0, 1, 0,
                     0, 0, 0, 1};
  bool horizon_problem = false;
  static bool do_once = true;

  if (do_once) {
    do_once = false;
    object3d_test_init ();
  }

  step_load_models (BOARD_THICKNESS);

//  printf ("Expose event at (%i,%i): %i x %i\n", ev->area.x, ev->area.y, ev->area.width, ev->area.height);
//  printf ("Event type %i, send_event %i\n", ev->type, ev->send_event);

  /* For some reason we get horrible glitching when we don't repaint the whole event region...
   * It also appears that we get spurious small repaints interleaved with full area re-paints,
   * so lets try just droping the small ones.
   * NB: Currently, all our programmatically triggered repaints cover the whole region.
   */
  if (ev->area.x != 0 || ev->area.y != 0)
    return FALSE;

  gtk_widget_get_allocation (widget, &allocation);

  ghid_start_drawing (port, widget);

  Output.fgGC = hid_draw_make_gc (&ghid_graphics);
  Output.bgGC = hid_draw_make_gc (&ghid_graphics);
  Output.pmGC = hid_draw_make_gc (&ghid_graphics);

  /* If we don't have any stencil bits available,
     we can't use the hidgl polygon drawing routine */
  /* TODO: We could use the GLU tessellator though */
  if (hidgl_stencil_bits (priv->hidgl) == 0)
    ghid_graphics_class.fill_pcb_polygon = common_fill_pcb_polygon;

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glViewport (0, 0, allocation.width, allocation.height);

  glEnable (GL_SCISSOR_TEST);
  glScissor (ev->area.x,
             allocation.height - ev->area.height - ev->area.y,
             ev->area.width, ev->area.height);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();

  aspect = (float)allocation.width / (float)allocation.height;

#ifdef VIEW_ORTHO
  glOrtho (-1. * aspect, 1. * aspect, 1., -1., 1., 24.);
#else
  glFrustum (-1. * aspect, 1 * aspect, 1., -1., 1., 24.);
#endif

  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();

#ifndef VIEW_ORTHO
  /* TEST HACK */
  glScalef (11., 11., 1.);
#endif

  /* Push the space coordinates board back into the middle of the z-view volume */
  /* XXX: THIS CAUSES NEED FOR SCALING BY 11 IN ghid_unproject_to_z_plane(), FOR ORTHO VIEW ONLY! */
  glTranslatef (0., 0., -11.);

  /* Rotate about the center of the board space */
  glMultMatrixf ((GLfloat *)view_matrix);

  /* Flip about the center of the viewed area */
  glScalef ((port->view.flip_x ? -1. : 1.),
            (port->view.flip_y ? -1. : 1.),
            ((port->view.flip_x == port->view.flip_y) ? 1. : -1.));

  /* Scale board coordiantes to (-1,-1)-(1,1) coordiantes */
  /* Adjust the "w" coordinate of our homogeneous coodinates. We coulld in
   * theory just use glScalef to transform, but on mesa this produces errors
   * as the resulting modelview matrix has a very small determinant.
   */
  scale[15] = port->view.coord_per_px * (float)MIN (widget->allocation.width, widget->allocation.height) / 2.;
  /* XXX: Need to choose which to use (width or height) based on the aspect of the window
   *      AND the aspect of the board!
   */
  glMultMatrixf (scale);

  /* Translate to the center of the board space view */
  glTranslatef (-SIDE_X (port->view.x0 + port->view.width / 2),
                -SIDE_Y (port->view.y0 + port->view.height / 2),
                0.);

  /* Stash the model view matrix so we can work out the screen coordinate -> board coordinate mapping */
  glGetFloatv (GL_MODELVIEW_MATRIX, (GLfloat *)last_modelview_matrix);
  glGetFloatv (GL_PROJECTION_MATRIX, (GLfloat *)last_projection_matrix);

#if 0
  /* Fix up matrix so the board Z coordinate does not affect world Z
   * this lets us view each stacked layer without parallax effects.
   *
   * Commented out because it breaks:
   *   Board view "which side should I render first" calculation
   *   Z-buffer depth occlusion when rendering component models
   */
  last_modelview_matrix[2][2] = 0.;
  glLoadMatrixf ((GLfloat *)last_modelview_matrix);
#endif

  glEnable (GL_STENCIL_TEST);
  glEnable (GL_DEPTH_TEST);
  glDepthFunc (GL_ALWAYS);
  glClearColor (port->offlimits_color.red / 65535.,
                port->offlimits_color.green / 65535.,
                port->offlimits_color.blue / 65535.,
                1.);
  glStencilMask (~0);
  glClearStencil (0);
  glClear (GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  hidgl_reset_stencil_usage (priv->hidgl);

  /* Disable the stencil test until we need it - otherwise it gets dirty */
  glDisable (GL_STENCIL_TEST);
  glStencilMask (0);
  glStencilFunc (GL_ALWAYS, 0, 0);

  /* Test the 8 corners of a cube spanning the event */
  min_depth = -50 + compute_depth (0);                    /* FIXME: NEED TO USE PHYSICAL GROUPS */
  max_depth =  50 + compute_depth (max_copper_layer - 1); /* FIXME: NEED TO USE PHYSICAL GROUPS */

  if (!ghid_unproject_to_z_plane (ev->area.x,
                                  ev->area.y,
                                  min_depth, &new_x, &new_y))
    horizon_problem = true;

  max_x = min_x = new_x;
  max_y = min_y = new_y;

  if (!ghid_unproject_to_z_plane (ev->area.x,
                                  ev->area.y,
                                  max_depth, &new_x, &new_y))
    horizon_problem = true;

  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  /* */
  if (!ghid_unproject_to_z_plane (ev->area.x + ev->area.width,
                                 ev->area.y,
                                 min_depth, &new_x, &new_y))
    horizon_problem = true;

  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  if (!ghid_unproject_to_z_plane (ev->area.x + ev->area.width,
                                  ev->area.y,
                                  max_depth, &new_x, &new_y))
    horizon_problem = true;

  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  /* */
  if (!ghid_unproject_to_z_plane (ev->area.x + ev->area.width,
                                  ev->area.y + ev->area.height,
                                  min_depth, &new_x, &new_y))
    horizon_problem = true;

  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  if (!ghid_unproject_to_z_plane (ev->area.x + ev->area.width,
                                  ev->area.y + ev->area.height,
                                  max_depth, &new_x, &new_y))
    horizon_problem = true;

  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  /* */
  if (!ghid_unproject_to_z_plane (ev->area.x,
                                  ev->area.y + ev->area.height,
                                  min_depth,
                                  &new_x, &new_y))
    horizon_problem = true;

  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  if (!ghid_unproject_to_z_plane (ev->area.x,
                                  ev->area.y + ev->area.height,
                                  max_depth,
                                  &new_x, &new_y))
    horizon_problem = true;

  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  if (horizon_problem) {
    min_x = 0;
    min_y = 0;
    max_x = PCB->MaxWidth;
    max_y = PCB->MaxHeight;
  }

  if (min_x < (double)-G_MAXINT / 4.) min_x = -G_MAXINT / 4;
  if (min_y < (double)-G_MAXINT / 4.) min_y = -G_MAXINT / 4;
  if (max_x < (double)-G_MAXINT / 4.) max_x = -G_MAXINT / 4;
  if (max_y < (double)-G_MAXINT / 4.) max_y = -G_MAXINT / 4;
  if (min_x > (double) G_MAXINT / 4.) min_x =  G_MAXINT / 4;
  if (min_y > (double) G_MAXINT / 4.) min_y =  G_MAXINT / 4;
  if (max_x > (double) G_MAXINT / 4.) max_x =  G_MAXINT / 4;
  if (max_y > (double) G_MAXINT / 4.) max_y =  G_MAXINT / 4;

  region.X1 = min_x;  region.X2 = max_x + 1;
  region.Y1 = min_y;  region.Y2 = max_y + 1;

  region.X1 = MAX (0, MIN (PCB->MaxWidth,  region.X1));
  region.X2 = MAX (0, MIN (PCB->MaxWidth,  region.X2));
  region.Y1 = MAX (0, MIN (PCB->MaxHeight, region.Y1));
  region.Y2 = MAX (0, MIN (PCB->MaxHeight, region.Y2));

  glColor3f (port->bg_color.red / 65535.,
             port->bg_color.green / 65535.,
             port->bg_color.blue / 65535.);

  ghid_invalidate_current_gc ();

  /* Setup stenciling */
  /* Drawing operations set the stencil buffer to '1' */
  glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE); /* Stencil pass => replace stencil value (with 1) */
  /* Drawing operations as masked to areas where the stencil buffer is '0' */
  /* glStencilFunc (GL_GREATER, 1, 1); */           /* Draw only where stencil buffer is 0 */

  glDepthMask (GL_FALSE);
  if (global_view_2d) {
    glBegin (GL_QUADS);
    glVertex3i (0,             0,              0);
    glVertex3i (PCB->MaxWidth, 0,              0);
    glVertex3i (PCB->MaxWidth, PCB->MaxHeight, 0);
    glVertex3i (0,             PCB->MaxHeight, 0);
    glEnd ();
  } else {
    int top_group;
    int bottom_group;
    int min_phys_group;
    int max_phys_group;
    int i;

    top_group = GetLayerGroupNumberBySide (TOP_SIDE);
    bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);

    min_phys_group = MIN (bottom_group, top_group);
    max_phys_group = MAX (bottom_group, top_group);

    glBegin (GL_QUADS);
    for (i = min_phys_group; i <= max_phys_group; i++) {
      int depth = compute_depth (i);
      glVertex3i (0,             0,              depth);
      glVertex3i (PCB->MaxWidth, 0,              depth);
      glVertex3i (PCB->MaxWidth, PCB->MaxHeight, depth);
      glVertex3i (0,             PCB->MaxHeight, depth);
    }
    glEnd ();
  }

  glDepthMask (GL_TRUE);

  ghid_draw_bg_image ();

  /* hid_expose_callback (&ghid_graphics, &region, 0); */
  ghid_draw_everything (&region);
  hidgl_flush_triangles (priv->hidgl);

  glTexCoord2f (0., 0.);
  glColor3f (1., 1., 1.);

  if (0) {
    double x, y;
    Coord z = max_depth;

    glBegin (GL_LINES);

    ghid_unproject_to_z_plane (ev->area.x, ev->area.y, z, &x, &y);
    glPushMatrix ();
    glLoadIdentity ();
    glVertex3f (0., 0., 0.);
    glPopMatrix ();
    glVertex3f (x, y, z);

    ghid_unproject_to_z_plane (ev->area.x, ev->area.y + ev->area.height, z, &x, &y);
    glPushMatrix ();
    glLoadIdentity ();
    glVertex3f (0., 0., 0.);
    glPopMatrix ();
    glVertex3f (x, y, z);

    ghid_unproject_to_z_plane (ev->area.x + ev->area.width, ev->area.y + ev->area.height, z, &x, &y);
    glPushMatrix ();
    glLoadIdentity ();
    glVertex3f (0., 0., 0.);
    glPopMatrix ();
    glVertex3f (x, y, z);

    ghid_unproject_to_z_plane (ev->area.x + ev->area.width, ev->area.y, z, &x, &y);
    glPushMatrix ();
    glLoadIdentity ();
    glVertex3f (0., 0., 0.);
    glPopMatrix ();
    glVertex3f (x, y, z);

    glEnd ();
  }

  /* Set the current depth to the right value for the layer we are editing */
  priv->edit_depth = compute_depth (GetLayerGroupNumberByNumber (INDEXOFCURRENT));
  hidgl_set_depth (Output.fgGC, priv->edit_depth);

  ghid_draw_grid (Output.fgGC, &region);

  ghid_invalidate_current_gc ();

  DrawAttached (Output.fgGC);
  DrawMark (Output.fgGC);
  hidgl_flush_triangles (priv->hidgl);

  if (ghidgui->debugged_polyarea != NULL) {
    PolygonType dummy_poly;
    dummy_poly.Clipped = ghidgui->debugged_polyarea;
    dummy_poly.Flags = NoFlags ();
    SET_FLAG (FULLPOLYFLAG, &dummy_poly);

    common_thindraw_pcb_polygon (Output.fgGC, &dummy_poly, &region);
    hidgl_flush_triangles (priv->hidgl);
  }

  glEnable (GL_LIGHTING);

  glShadeModel (GL_SMOOTH);

  glEnable (GL_LIGHT0);

  /* XXX: FIX OUR NORMALS */
//  glEnable (GL_NORMALIZE);
//  glEnable (GL_RESCALE_NORMAL);

  glDepthFunc (GL_LESS);
  glDisable (GL_STENCIL_TEST);

  glEnable (GL_CULL_FACE); /* XXX: Fix model face filling */
  glCullFace (GL_BACK);

  glEnable (GL_COLOR_MATERIAL);

  // Front material ambient and diffuse colors track glColor
  glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);

  if (1) {
    GLfloat global_ambient[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glLightModelfv (GL_LIGHT_MODEL_AMBIENT, global_ambient);
    glLightModeli (GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
    glLightModeli (GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
  }
  if (1) {
    GLfloat diffuse[] =  {0.6, 0.6, 0.6, 1.0};
    GLfloat ambient[] =  {0.4, 0.4, 0.4, 1.0};
    GLfloat specular[] = {0.4, 0.4, 0.4, 1.0};
    glLightfv (GL_LIGHT0, GL_DIFFUSE,  diffuse);
    glLightfv (GL_LIGHT0, GL_AMBIENT,  ambient);
    glLightfv (GL_LIGHT0, GL_SPECULAR, specular);
  }
  if (1) {
//    GLfloat position[] = {1., -1., 1., 0.};
//    GLfloat position[] = {1., -0.5, 1., 0.};
//    GLfloat position[] = {0., -1., 1., 0.};
//    GLfloat position[] = {0.5, -1., 1., 0.};
//    GLfloat position[] = {0.0, -0.5, 1., 0.};
    GLfloat position[] = {0.0, 0.0, 1., 0.};
//    GLfloat position[] = {0.0, 0.0, 10., 1.};
    GLfloat abspos = sqrt (position[0] * position[0] +
                           position[1] * position[1] +
                           position[2] * position[2]);
//    position[0] /= abspos;
//    position[1] /= abspos;
//    position[2] /= abspos;
    glPushMatrix ();
    glLoadIdentity ();
    glLightfv (GL_LIGHT0, GL_POSITION, position);
    glPopMatrix ();
  }

//  glDisable (GL_DEPTH_TEST); /* TEST */
//  glDepthMask (FALSE); /* TEST */

  if (!global_view_2d)
    ghid_draw_packages (&region);

  glDepthMask (TRUE); /* TEST */

  glDisable (GL_CULL_FACE);
  glDisable (GL_DEPTH_TEST);
  glDisable (GL_LIGHT0);
  glDisable (GL_COLOR_MATERIAL);
  glDisable (GL_LIGHTING);

  draw_crosshair (Output.fgGC, priv);
  //object3d_draw_debug (Output.fgGC);
  if (step_read_test != NULL)
    object3d_draw (Output.fgGC, step_read_test, false);

  hidgl_flush_triangles (priv->hidgl);

  draw_lead_user (Output.fgGC, priv);

  ghid_end_drawing (port, widget);

  hid_draw_destroy_gc (Output.fgGC);
  hid_draw_destroy_gc (Output.bgGC);
  hid_draw_destroy_gc (Output.pmGC);

  Output.fgGC = NULL;
  Output.bgGC = NULL;
  Output.pmGC = NULL;
  g_timer_start (priv->time_since_expose);

  return FALSE;
}

/* This realize callback is used to work around a crash bug in some mesa
 * versions (observed on a machine running the intel i965 driver. It isn't
 * obvious why it helps, but somehow fiddling with the GL context here solves
 * the issue. The problem appears to have been fixed in recent mesa versions.
 */
void
ghid_port_drawing_realize_cb (GtkWidget *widget, gpointer data)
{
  GdkGLContext *glcontext = gtk_widget_get_gl_context (widget);
  GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);

  if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext))
    return;

  gdk_gl_drawable_gl_end (gldrawable);
  return;
}

gboolean
ghid_pinout_preview_expose (GtkWidget *widget,
                            GdkEventExpose *ev)
{
  GhidPinoutPreview *pinout = GHID_PINOUT_PREVIEW (widget);
  render_priv *priv = gport->render_priv;
  GtkAllocation allocation;
  view_data save_view;
  int save_width, save_height;
  Coord save_max_width;
  Coord save_max_height;
  double xz, yz;

  save_view = gport->view;
  save_width = gport->width;
  save_height = gport->height;
  save_max_width = PCB->MaxWidth;
  save_max_height = PCB->MaxHeight;

  /* Setup zoom factor for drawing routines */

  gtk_widget_get_allocation (widget, &allocation);
  xz = (double) pinout->x_max / allocation.width;
  yz = (double) pinout->y_max / allocation.height;
  if (xz > yz)
    gport->view.coord_per_px = xz;
  else
    gport->view.coord_per_px = yz;

  gport->width = allocation.width;
  gport->height = allocation.height;
  gport->view.width = allocation.width * gport->view.coord_per_px;
  gport->view.height = allocation.height * gport->view.coord_per_px;
  gport->view.x0 = (pinout->x_max - gport->view.width) / 2;
  gport->view.y0 = (pinout->y_max - gport->view.height) / 2;
  PCB->MaxWidth = pinout->x_max;
  PCB->MaxHeight = pinout->y_max;

  ghid_start_drawing (gport, widget);

#if 0  /* We disable alpha blending here, as hid_expose_callback() does not
        * call set_layer() as appropriate for us to sub-composite rendering
        * from each layer when drawing a single element. If we leave alpha-
        * blending on, it means text and overlapping pads are rendered ugly.
        */

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

  glViewport (0, 0, allocation.width, allocation.height);

#if 0  /* We disable the scissor test here, as it is interacting badly with
        * being handed expose events which don't cover the whole window.
        * As we have a double-buffered GL window, we end up with unintialised
        * contents remaining in the unpainted areas (outside the scissor
        * region), and these are being flipped onto the screen.
        *
        * The debugging code below shows multiple expose events when the
        * window is shown the first time, some of which are very small.
        *
        * XXX: There is clearly a perforamnce issue here, in that we may
        *      be rendering the preview more times, and over a larger area
        *      than is really required.
        */

  glEnable (GL_SCISSOR_TEST);
  glScissor (ev->area.x,
             allocation.height - ev->area.height - ev->area.y,
             ev->area.width, ev->area.height);
#endif

#ifdef DEBUG
  printf ("EVT: %i, %i, w=%i, h=%i, Scissor setup: glScissor (%f, %f, %f, %f);\n",
          ev->area.x, ev->area.y, ev->area.width, ev->area.height,
             (double)ev->area.x,
             (double)(allocation.height - ev->area.height - ev->area.y),
             (double)ev->area.width,
             (double)ev->area.height);
#endif

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (0, allocation.width, allocation.height, 0, -100000, 100000);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  glTranslatef (0.0f, 0.0f, -Z_NEAR);

  glClearColor (gport->bg_color.red / 65535.,
                gport->bg_color.green / 65535.,
                gport->bg_color.blue / 65535.,
                1.);
  glStencilMask (~0);
  glClearStencil (0);
  glClear (GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  hidgl_reset_stencil_usage (priv->hidgl);

  /* Disable the stencil test until we need it - otherwise it gets dirty */
  glDisable (GL_STENCIL_TEST);
  glStencilMask (0);
  glStencilFunc (GL_ALWAYS, 0, 0);

  /* call the drawing routine */
  ghid_invalidate_current_gc ();
  glPushMatrix ();
  glScalef ((gport->view.flip_x ? -1. : 1.) / gport->view.coord_per_px,
            (gport->view.flip_y ? -1. : 1.) / gport->view.coord_per_px,
            ((gport->view.flip_x == gport->view.flip_y) ? 1. : -1.) / gport->view.coord_per_px);
  glTranslatef (gport->view.flip_x ? gport->view.x0 - PCB->MaxWidth  :
                                    -gport->view.x0,
                gport->view.flip_y ? gport->view.y0 - PCB->MaxHeight :
                                    -gport->view.y0, 0);

  hid_expose_callback (&ghid_graphics, NULL, pinout->element);
  hidgl_flush_triangles (priv->hidgl);
  glPopMatrix ();

  ghid_end_drawing (gport, widget);

  gport->view = save_view;
  gport->width = save_width;
  gport->height = save_height;
  PCB->MaxWidth = save_max_width;
  PCB->MaxHeight = save_max_height;

  return FALSE;
}


GdkPixmap *
ghid_render_pixmap (int cx, int cy, double zoom, int width, int height, int depth)
{
  render_priv *priv = gport->render_priv;
  GdkGLConfig *glconfig;
  GdkPixmap *pixmap;
  GdkGLPixmap *glpixmap;
  GdkGLContext *drawarea_glcontext;
  GdkGLContext* glcontext;
  GdkGLDrawable* gldrawable;
  view_data save_view;
  int save_width, save_height;
  BoxType region;

  save_view = gport->view;
  save_width = gport->width;
  save_height = gport->height;

  /* Setup rendering context for drawing routines
   */

  /* NB: We share with the main rendering context so we can use the
   *     same pixel shader etc..
   */
  drawarea_glcontext = gtk_widget_get_gl_context (gport->drawing_area);

  glconfig = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGB     |
                                        GDK_GL_MODE_STENCIL |
                                        GDK_GL_MODE_DEPTH   |
                                        GDK_GL_MODE_SINGLE);

  pixmap = gdk_pixmap_new (NULL, width, height, depth);
  glpixmap = gdk_pixmap_set_gl_capability (pixmap, glconfig, NULL);
  gldrawable = GDK_GL_DRAWABLE (glpixmap);
  glcontext = gdk_gl_context_new (gldrawable, drawarea_glcontext,
                                  TRUE, GDK_GL_RGBA_TYPE);

  /* Setup zoom factor for drawing routines */

  gport->view.coord_per_px = zoom;
  gport->width = width;
  gport->height = height;
  gport->view.width = width * gport->view.coord_per_px;
  gport->view.height = height * gport->view.coord_per_px;
  gport->view.x0 = gport->view.flip_x ? PCB->MaxWidth - cx : cx;
  gport->view.x0 -= gport->view.height / 2;
  gport->view.y0 = gport->view.flip_y ? PCB->MaxHeight - cy : cy;
  gport->view.y0 -= gport->view.width / 2;

  /* make GL-context "current" */
  if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext)) {
    return NULL;
  }
  hidgl_start_render (priv->hidgl);
  gport->render_priv->in_context = true;

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glViewport (0, 0, width, height);

  glEnable (GL_SCISSOR_TEST);
  glScissor (0, 0, width, height);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (0, width, height, 0, -100000, 100000);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  glTranslatef (0.0f, 0.0f, -Z_NEAR);

  glClearColor (gport->bg_color.red / 65535.,
                gport->bg_color.green / 65535.,
                gport->bg_color.blue / 65535.,
                1.);
  glStencilMask (~0);
  glClearStencil (0);
  glClear (GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  hidgl_reset_stencil_usage (priv->hidgl);

  /* Disable the stencil test until we need it - otherwise it gets dirty */
  glDisable (GL_STENCIL_TEST);
  glStencilMask (0);
  glStencilFunc (GL_ALWAYS, 0, 0);

  /* call the drawing routine */
  ghid_invalidate_current_gc ();
  glPushMatrix ();
  glScalef ((gport->view.flip_x ? -1. : 1.) / gport->view.coord_per_px,
            (gport->view.flip_y ? -1. : 1.) / gport->view.coord_per_px,
            ((gport->view.flip_x == gport->view.flip_y) ? 1. : -1.) / gport->view.coord_per_px);
  glTranslatef (gport->view.flip_x ? gport->view.x0 - PCB->MaxWidth  :
                                    -gport->view.x0,
                gport->view.flip_y ? gport->view.y0 - PCB->MaxHeight :
                                    -gport->view.y0, 0);

  region.X1 = MIN(Px(0), Px(gport->width + 1));
  region.Y1 = MIN(Py(0), Py(gport->height + 1));
  region.X2 = MAX(Px(0), Px(gport->width + 1));
  region.Y2 = MAX(Py(0), Py(gport->height + 1));

  region.X1 = MAX (0, MIN (PCB->MaxWidth,  region.X1));
  region.X2 = MAX (0, MIN (PCB->MaxWidth,  region.X2));
  region.Y1 = MAX (0, MIN (PCB->MaxHeight, region.Y1));
  region.Y2 = MAX (0, MIN (PCB->MaxHeight, region.Y2));

  hid_expose_callback (&ghid_graphics, &region, NULL);
  hidgl_flush_triangles (priv->hidgl);
  glPopMatrix ();

  glFlush ();

  hidgl_finish_render (priv->hidgl);

  /* end drawing to current GL-context */
  gport->render_priv->in_context = false;
  gdk_gl_drawable_gl_end (gldrawable);

  gdk_pixmap_unset_gl_capability (pixmap);

  g_object_unref (glconfig);
  g_object_unref (glcontext);

  gport->view = save_view;
  gport->width = save_width;
  gport->height = save_height;

  return pixmap;
}

HID_DRAW *
ghid_request_debug_draw (void)
{
  GHidPort *port = gport;
  GtkWidget *widget = port->drawing_area;
  GtkAllocation allocation;

  gtk_widget_get_allocation (widget, &allocation);

  ghid_start_drawing (port, widget);

  glViewport (0, 0, allocation.width, allocation.height);

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (0, allocation.width, allocation.height, 0, 0, 100);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  glTranslatef (0.0f, 0.0f, -Z_NEAR);

  ghid_invalidate_current_gc ();

  /* Setup stenciling */
  glDisable (GL_STENCIL_TEST);

  glPushMatrix ();
  glScalef ((port->view.flip_x ? -1. : 1.) / port->view.coord_per_px,
            (port->view.flip_y ? -1. : 1.) / port->view.coord_per_px,
            ((gport->view.flip_x == port->view.flip_y) ? 1. : -1.) / gport->view.coord_per_px);
  glTranslatef (port->view.flip_x ? port->view.x0 - PCB->MaxWidth  :
                             -port->view.x0,
                port->view.flip_y ? port->view.y0 - PCB->MaxHeight :
                             -port->view.y0, 0);

  return &ghid_graphics;
}

void
ghid_flush_debug_draw (void)
{
  render_priv *priv = gport->render_priv;
  GtkWidget *widget = gport->drawing_area;
  GdkGLDrawable *pGlDrawable = gtk_widget_get_gl_drawable (widget);

  hidgl_flush_triangles (priv->hidgl);

  if (gdk_gl_drawable_is_double_buffered (pGlDrawable))
    gdk_gl_drawable_swap_buffers (pGlDrawable);
  else
    glFlush ();
}

void
ghid_finish_debug_draw (void)
{
  render_priv *priv = gport->render_priv;

  hidgl_flush_triangles (priv->hidgl);
  glPopMatrix ();

  ghid_end_drawing (gport, gport->drawing_area);
}

static double
determinant_2x2 (double m[2][2])
{
  double det;
  det = m[0][0] * m[1][1] -
        m[0][1] * m[1][0];
  return det;
}

#if 0
static float
determinant_4x4 (float m[4][4])
{
  float det;
  det = m[0][3] * m[1][2] * m[2][1] * m[3][0]-m[0][2] * m[1][3] * m[2][1] * m[3][0] -
        m[0][3] * m[1][1] * m[2][2] * m[3][0]+m[0][1] * m[1][3] * m[2][2] * m[3][0] +
        m[0][2] * m[1][1] * m[2][3] * m[3][0]-m[0][1] * m[1][2] * m[2][3] * m[3][0] -
        m[0][3] * m[1][2] * m[2][0] * m[3][1]+m[0][2] * m[1][3] * m[2][0] * m[3][1] +
        m[0][3] * m[1][0] * m[2][2] * m[3][1]-m[0][0] * m[1][3] * m[2][2] * m[3][1] -
        m[0][2] * m[1][0] * m[2][3] * m[3][1]+m[0][0] * m[1][2] * m[2][3] * m[3][1] +
        m[0][3] * m[1][1] * m[2][0] * m[3][2]-m[0][1] * m[1][3] * m[2][0] * m[3][2] -
        m[0][3] * m[1][0] * m[2][1] * m[3][2]+m[0][0] * m[1][3] * m[2][1] * m[3][2] +
        m[0][1] * m[1][0] * m[2][3] * m[3][2]-m[0][0] * m[1][1] * m[2][3] * m[3][2] -
        m[0][2] * m[1][1] * m[2][0] * m[3][3]+m[0][1] * m[1][2] * m[2][0] * m[3][3] +
        m[0][2] * m[1][0] * m[2][1] * m[3][3]-m[0][0] * m[1][2] * m[2][1] * m[3][3] -
        m[0][1] * m[1][0] * m[2][2] * m[3][3]+m[0][0] * m[1][1] * m[2][2] * m[3][3];
   return det;
}
#endif

static void
invert_2x2 (double m[2][2], double out[2][2])
{
  double scale = 1 / determinant_2x2 (m);
  out[0][0] =  m[1][1] * scale;
  out[0][1] = -m[0][1] * scale;
  out[1][0] = -m[1][0] * scale;
  out[1][1] =  m[0][0] * scale;
}

#if 0
static void
invert_4x4 (float m[4][4], float out[4][4])
{
  float scale = 1 / determinant_4x4 (m);

  out[0][0] = (m[1][2] * m[2][3] * m[3][1] - m[1][3] * m[2][2] * m[3][1] +
               m[1][3] * m[2][1] * m[3][2] - m[1][1] * m[2][3] * m[3][2] -
               m[1][2] * m[2][1] * m[3][3] + m[1][1] * m[2][2] * m[3][3]) * scale;
  out[0][1] = (m[0][3] * m[2][2] * m[3][1] - m[0][2] * m[2][3] * m[3][1] -
               m[0][3] * m[2][1] * m[3][2] + m[0][1] * m[2][3] * m[3][2] +
               m[0][2] * m[2][1] * m[3][3] - m[0][1] * m[2][2] * m[3][3]) * scale;
  out[0][2] = (m[0][2] * m[1][3] * m[3][1] - m[0][3] * m[1][2] * m[3][1] +
               m[0][3] * m[1][1] * m[3][2] - m[0][1] * m[1][3] * m[3][2] -
               m[0][2] * m[1][1] * m[3][3] + m[0][1] * m[1][2] * m[3][3]) * scale;
  out[0][3] = (m[0][3] * m[1][2] * m[2][1] - m[0][2] * m[1][3] * m[2][1] -
               m[0][3] * m[1][1] * m[2][2] + m[0][1] * m[1][3] * m[2][2] +
               m[0][2] * m[1][1] * m[2][3] - m[0][1] * m[1][2] * m[2][3]) * scale;
  out[1][0] = (m[1][3] * m[2][2] * m[3][0] - m[1][2] * m[2][3] * m[3][0] -
               m[1][3] * m[2][0] * m[3][2] + m[1][0] * m[2][3] * m[3][2] +
               m[1][2] * m[2][0] * m[3][3] - m[1][0] * m[2][2] * m[3][3]) * scale;
  out[1][1] = (m[0][2] * m[2][3] * m[3][0] - m[0][3] * m[2][2] * m[3][0] +
               m[0][3] * m[2][0] * m[3][2] - m[0][0] * m[2][3] * m[3][2] -
               m[0][2] * m[2][0] * m[3][3] + m[0][0] * m[2][2] * m[3][3]) * scale;
  out[1][2] = (m[0][3] * m[1][2] * m[3][0] - m[0][2] * m[1][3] * m[3][0] -
               m[0][3] * m[1][0] * m[3][2] + m[0][0] * m[1][3] * m[3][2] +
               m[0][2] * m[1][0] * m[3][3] - m[0][0] * m[1][2] * m[3][3]) * scale;
  out[1][3] = (m[0][2] * m[1][3] * m[2][0] - m[0][3] * m[1][2] * m[2][0] +
               m[0][3] * m[1][0] * m[2][2] - m[0][0] * m[1][3] * m[2][2] -
               m[0][2] * m[1][0] * m[2][3] + m[0][0] * m[1][2] * m[2][3]) * scale;
  out[2][0] = (m[1][1] * m[2][3] * m[3][0] - m[1][3] * m[2][1] * m[3][0] +
               m[1][3] * m[2][0] * m[3][1] - m[1][0] * m[2][3] * m[3][1] -
               m[1][1] * m[2][0] * m[3][3] + m[1][0] * m[2][1] * m[3][3]) * scale;
  out[2][1] = (m[0][3] * m[2][1] * m[3][0] - m[0][1] * m[2][3] * m[3][0] -
               m[0][3] * m[2][0] * m[3][1] + m[0][0] * m[2][3] * m[3][1] +
               m[0][1] * m[2][0] * m[3][3] - m[0][0] * m[2][1] * m[3][3]) * scale;
  out[2][2] = (m[0][1] * m[1][3] * m[3][0] - m[0][3] * m[1][1] * m[3][0] +
               m[0][3] * m[1][0] * m[3][1] - m[0][0] * m[1][3] * m[3][1] -
               m[0][1] * m[1][0] * m[3][3] + m[0][0] * m[1][1] * m[3][3]) * scale;
  out[2][3] = (m[0][3] * m[1][1] * m[2][0] - m[0][1] * m[1][3] * m[2][0] -
               m[0][3] * m[1][0] * m[2][1] + m[0][0] * m[1][3] * m[2][1] +
               m[0][1] * m[1][0] * m[2][3] - m[0][0] * m[1][1] * m[2][3]) * scale;
  out[3][0] = (m[1][2] * m[2][1] * m[3][0] - m[1][1] * m[2][2] * m[3][0] -
               m[1][2] * m[2][0] * m[3][1] + m[1][0] * m[2][2] * m[3][1] +
               m[1][1] * m[2][0] * m[3][2] - m[1][0] * m[2][1] * m[3][2]) * scale;
  out[3][1] = (m[0][1] * m[2][2] * m[3][0] - m[0][2] * m[2][1] * m[3][0] +
               m[0][2] * m[2][0] * m[3][1] - m[0][0] * m[2][2] * m[3][1] -
               m[0][1] * m[2][0] * m[3][2] + m[0][0] * m[2][1] * m[3][2]) * scale;
  out[3][2] = (m[0][2] * m[1][1] * m[3][0] - m[0][1] * m[1][2] * m[3][0] -
               m[0][2] * m[1][0] * m[3][1] + m[0][0] * m[1][2] * m[3][1] +
               m[0][1] * m[1][0] * m[3][2] - m[0][0] * m[1][1] * m[3][2]) * scale;
  out[3][3] = (m[0][1] * m[1][2] * m[2][0] - m[0][2] * m[1][1] * m[2][0] +
               m[0][2] * m[1][0] * m[2][1] - m[0][0] * m[1][2] * m[2][1] -
               m[0][1] * m[1][0] * m[2][2] + m[0][0] * m[1][1] * m[2][2]) * scale;
}
#endif


static bool
ghid_unproject_to_z_plane (int ex, int ey, Coord pcb_z, double *pcb_x, double *pcb_y)
{
  double mat[2][2];
  double inv_mat[2][2];
  double x, y;
  double fvz;
  double vpx, vpy;
  double fvx, fvy;
  GtkWidget *widget = gport->drawing_area;

  /* FIXME: Dirty kludge.. I know what our view parameters are here */
  double aspect = (double)widget->allocation.width / (double)widget->allocation.height;
  double width = 2. * aspect;
  double height = 2.;
  double near_plane = 1.;
  /* double far_plane = 24.; */

  /* This is nasty beyond words, but I'm lazy and translating directly
   * from some untested maths I derived which used this notation */
  double A, B, C, D, E, F, G, H, I, J, K, L;

  /* NB: last_modelview_matrix is transposed in memory! */
  A = last_modelview_matrix[0][0];
  B = last_modelview_matrix[1][0];
  C = last_modelview_matrix[2][0];
  D = last_modelview_matrix[3][0];
  E = last_modelview_matrix[0][1];
  F = last_modelview_matrix[1][1];
  G = last_modelview_matrix[2][1];
  H = last_modelview_matrix[3][1];
  I = last_modelview_matrix[0][2];
  J = last_modelview_matrix[1][2];
  K = last_modelview_matrix[2][2];
  L = last_modelview_matrix[3][2];

  /* I could assert that the last row is (as assumed) [0 0 0 1], but again.. I'm lazy */

  /* XXX: Actually the last element is not 1, we use it to scale the PCB coordinates! */
#if 0
  printf ("row %f %f %f %f\n", last_modelview_matrix[0][0],
                               last_modelview_matrix[1][0],
                               last_modelview_matrix[2][0],
                               last_modelview_matrix[3][0]);
  printf ("row %f %f %f %f\n", last_modelview_matrix[0][1],
                               last_modelview_matrix[1][1],
                               last_modelview_matrix[2][1],
                               last_modelview_matrix[3][1]);
  printf ("row %f %f %f %f\n", last_modelview_matrix[0][2],
                               last_modelview_matrix[1][2],
                               last_modelview_matrix[2][2],
                               last_modelview_matrix[3][2]);
  printf ("Last row %f %f %f %f\n", last_modelview_matrix[0][3],
                                    last_modelview_matrix[1][3],
                                    last_modelview_matrix[2][3],
                                    last_modelview_matrix[3][3]);
#endif

/*
    ex = view_matrix[0][0] * vx +
         view_matrix[0][1] * vy +
         view_matrix[0][2] * vz +
         view_matrix[0][3] * 1;
    ey = view_matrix[1][0] * vx +
         view_matrix[1][1] * vy +
         view_matrix[1][2] * vz +
         view_matrix[1][3] * 1;
    UNKNOWN ez = view_matrix[2][0] * vx +
                 view_matrix[2][1] * vy +
                 view_matrix[2][2] * vz +
                 view_matrix[2][3] * 1;
    UNKNOWN ew = view_matrix[3][0] * vx +
                 view_matrix[3][1] * vy +
                 view_matrix[3][2] * vz +
                 view_matrix[3][3] * 1;
*/

  /* Convert from event coordinates to viewport coordinates */
  vpx = (float)ex / (float)widget->allocation.width * 2. - 1.;
  vpy = (float)ey / (float)widget->allocation.height * 2. - 1.;

#ifdef VIEW_ORTHO
  /* XXX: NOT SURE WHAT IS WRONG TO REQUIRE THIS... SOMETHING IS INCONSISTENT */
  vpx /= 11.;
  vpy /= 11.;
#endif

  /* Convert our model space Z plane coordinte to float for convenience */
  fvz = (float)pcb_z;

  /* This isn't really X and Y? */
  x = (C * fvz + D) * 2. / width  * near_plane + vpx * (K * fvz + L);
  y = (G * fvz + H) * 2. / height * near_plane + vpy * (K * fvz + L);

  mat[0][0] = -vpx * I - A * 2 / width / near_plane;
  mat[0][1] = -vpx * J - B * 2 / width / near_plane;
  mat[1][0] = -vpy * I - E * 2 / height / near_plane;
  mat[1][1] = -vpy * J - F * 2 / height / near_plane;

//  if (fabs (determinant_2x2 (mat)) < 0.000000000001)
//    printf ("Determinant is quite small\n");

  invert_2x2 (mat, inv_mat);

  fvx = (inv_mat[0][0] * x + inv_mat[0][1] * y);
  fvy = (inv_mat[1][0] * x + inv_mat[1][1] * y);

//  if (fvx == NAN) printf ("fvx is NAN\n");
//  if (fvy == NAN) printf ("fvx is NAN\n");

//  if (fabs (fvx) == INFINITY) printf ("fvx is infinite %f\n", fvx);
//  if (fabs (fvy) == INFINITY) printf ("fvy is infinite %f\n", fvy);

//  if (fvx > (double)G_MAXINT/5.) {fvx = (double)G_MAXINT/5.; printf ("fvx overflow clamped\n"); }
//  if (fvy > (double)G_MAXINT/5.) {fvy = (double)G_MAXINT/5.; printf ("fvy overflow clamped\n"); }

//  if (fvx < (double)-G_MAXINT/5.) {fvx = (double)-G_MAXINT/5.; printf ("fvx underflow clamped\n"); }
//  if (fvy < (double)-G_MAXINT/5.) {fvy = (double)-G_MAXINT/5.; printf ("fvy underflow clamped\n"); }

//  *pcb_x = (Coord)fvx;
//  *pcb_y = (Coord)fvy;

  *pcb_x = fvx;
  *pcb_y = fvy;

  {
    /* Reproject the computed board plane coordinates to eye space */
    /* float ex = last_modelview_matrix[0][0] * fvx + last_modelview_matrix[1][0] * fvy + last_modelview_matrix[2][0] * fvz + last_modelview_matrix[3][0]; */
    /* float ey = last_modelview_matrix[0][1] * fvx + last_modelview_matrix[1][1] * fvy + last_modelview_matrix[2][1] * fvz + last_modelview_matrix[3][1]; */
    float ez = last_modelview_matrix[0][2] * fvx + last_modelview_matrix[1][2] * fvy + last_modelview_matrix[2][2] * fvz + last_modelview_matrix[3][2];
    /* We don't care about ew, as we don't use anything other than 1 for homogeneous coordinates at this stage */
    /* float ew = last_modelview_matrix[0][3] * fvx + last_modelview_matrix[1][3] * fvy + last_modelview_matrix[2][3] * fvz + last_modelview_matrix[3][3]; */

#if 0
    if (-ez < near_plane)
      printf ("ez is closer than the near_plane clipping plane, ez = %f\n", ez);
    if (-ez > far_plane)
      printf ("ez is further than the near_plane clipping plane, ez = %f\n", ez);
#endif
    if (-ez < 0) {
      // printf ("EZ IS BEHIND THE CAMERA !! ez = %f\n", ez);
      return false;
    }

    return true;
  }
}


bool
ghid_event_to_pcb_coords (int event_x, int event_y, Coord *pcb_x, Coord *pcb_y)
{
  render_priv *priv = gport->render_priv;
  double tmp_x, tmp_y;
  bool retval;

  retval = ghid_unproject_to_z_plane (event_x, event_y, priv->edit_depth, &tmp_x, &tmp_y);

  *pcb_x = tmp_x;
  *pcb_y = tmp_y;

  return retval;
}

bool
ghid_pcb_to_event_coords (Coord pcb_x, Coord pcb_y, int *event_x, int *event_y)
{
  render_priv *priv = gport->render_priv;
  float vpx, vpy, vpz, vpw;
  float wx, wy, ww;

  /* Transform the passed coordinate to eye space */

  /* NB: last_modelview_matrix is transposed in memory */
  vpx = last_modelview_matrix[0][0] * (float)pcb_x +
        last_modelview_matrix[1][0] * (float)pcb_y +
        last_modelview_matrix[2][0] * priv->edit_depth +
        last_modelview_matrix[3][0] * 1.;
  vpy = last_modelview_matrix[0][1] * (float)pcb_x +
        last_modelview_matrix[1][1] * (float)pcb_y +
        last_modelview_matrix[2][1] * priv->edit_depth +
        last_modelview_matrix[3][1] * 1.;
  vpz = last_modelview_matrix[0][2] * (float)pcb_x +
        last_modelview_matrix[1][2] * (float)pcb_y +
        last_modelview_matrix[2][2] * priv->edit_depth +
        last_modelview_matrix[3][2] * 1.;
  vpw = last_modelview_matrix[0][3] * (float)pcb_x +
        last_modelview_matrix[1][3] * (float)pcb_y +
        last_modelview_matrix[2][3] * priv->edit_depth +
        last_modelview_matrix[3][3] * 1.;

  /* Project the eye coordinates into clip space */

  /* NB: last_projection_matrix is transposed in memory */
  wx = last_projection_matrix[0][0] * vpx +
       last_projection_matrix[1][0] * vpy +
       last_projection_matrix[2][0] * vpz +
       last_projection_matrix[3][0] * vpw;
  wy = last_projection_matrix[0][1] * vpx +
       last_projection_matrix[1][1] * vpy +
       last_projection_matrix[2][1] * vpz +
       last_projection_matrix[3][1] * vpw;
  ww = last_projection_matrix[0][3] * vpx +
       last_projection_matrix[1][3] * vpy +
       last_projection_matrix[2][3] * vpz +
       last_projection_matrix[3][3] * vpw;

  /* And transform according to our viewport */
  *event_x = ( wx / ww + 1.) * 0.5 * (float)gport->drawing_area->allocation.width,
  *event_y = (-wy / ww + 1.) * 0.5 * (float)gport->drawing_area->allocation.height;

  return true;
}

void
ghid_view_2d (void *ball, gboolean view_2d, gpointer userdata)
{
  global_view_2d = view_2d;

  if (view_2d)
    {
      ghid_graphics_class.fill_pcb_pv = ghid_fill_pcb_pv_2d; /* 2D, BB Via aware (with layer end-point annotations) */
    }
  else
    {
      ghid_graphics_class.fill_pcb_pv = common_fill_pcb_pv; /* Physical model only */
    }

  ghid_invalidate_all ();
}

void
ghid_port_rotate (void *ball, float *quarternion, gpointer userdata)
{
#ifdef DEBUG_ROTATE
  int row, column;
#endif

  build_rotmatrix (view_matrix, quarternion);

#ifdef DEBUG_ROTATE
  for (row = 0; row < 4; row++) {
    printf ("[ %f", view_matrix[row][0]);
    for (column = 1; column < 4; column++) {
      printf (",\t%f", view_matrix[row][column]);
    }
    printf ("\t]\n");
  }
  printf ("\n");
#endif

  ghid_invalidate_all ();
}


#define LEAD_USER_WIDTH           0.2          /* millimeters */
#define LEAD_USER_PERIOD          (1000 / 20)  /* 20fps (in ms) */
#define LEAD_USER_VELOCITY        3.           /* millimeters per second */
#define LEAD_USER_ARC_COUNT       3
#define LEAD_USER_ARC_SEPARATION  3.           /* millimeters */
#define LEAD_USER_INITIAL_RADIUS  10.          /* millimetres */
#define LEAD_USER_COLOR_R         1.
#define LEAD_USER_COLOR_G         1.
#define LEAD_USER_COLOR_B         0.

static void
draw_lead_user (hidGC gc, render_priv *priv)
{
  gtkGC gtk_gc = (gtkGC)gc;
  int i;
  double radius = priv->lead_user_radius;
  double width = MM_TO_COORD (LEAD_USER_WIDTH);
  double separation = MM_TO_COORD (LEAD_USER_ARC_SEPARATION);

  if (!priv->lead_user)
    return;

  glPushAttrib (GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
  glEnable (GL_COLOR_LOGIC_OP);
  glLogicOp (GL_XOR);
  glColor3f (LEAD_USER_COLOR_R, LEAD_USER_COLOR_G,LEAD_USER_COLOR_B);


  /* arcs at the approrpriate radii */

  for (i = 0; i < LEAD_USER_ARC_COUNT; i++, radius -= separation)
    {
      if (radius < width)
        radius += MM_TO_COORD (LEAD_USER_INITIAL_RADIUS);

      /* Draw an arc at radius */
      hidgl_draw_arc (gc, width, priv->lead_user_x, priv->lead_user_y,
                      radius, radius, 0, 360, gport->view.coord_per_px);
    }

  hidgl_flush_triangles (gtk_gc->hidgl_gc.hidgl);
  glPopAttrib ();
}

gboolean
lead_user_cb (gpointer data)
{
  render_priv *priv = data;
  Coord step;
  double elapsed_time;

  /* Queue a redraw */
  ghid_invalidate_all ();

  /* Update radius */
  elapsed_time = g_timer_elapsed (priv->lead_user_timer, NULL);
  g_timer_start (priv->lead_user_timer);

  step = MM_TO_COORD (LEAD_USER_VELOCITY * elapsed_time);
  if (priv->lead_user_radius > step)
    priv->lead_user_radius -= step;
  else
    priv->lead_user_radius = MM_TO_COORD (LEAD_USER_INITIAL_RADIUS);

  return TRUE;
}

void
ghid_lead_user_to_location (Coord x, Coord y)
{
  render_priv *priv = gport->render_priv;

  ghid_cancel_lead_user ();

  priv->lead_user = true;
  priv->lead_user_x = x;
  priv->lead_user_y = y;
  priv->lead_user_radius = MM_TO_COORD (LEAD_USER_INITIAL_RADIUS);
  priv->lead_user_timeout = g_timeout_add (LEAD_USER_PERIOD, lead_user_cb, priv);
  priv->lead_user_timer = g_timer_new ();
}

void
ghid_cancel_lead_user (void)
{
  render_priv *priv = gport->render_priv;

  if (priv->lead_user_timeout)
    g_source_remove (priv->lead_user_timeout);

  if (priv->lead_user_timer)
    g_timer_destroy (priv->lead_user_timer);

  if (priv->lead_user)
    ghid_invalidate_all ();

  priv->lead_user_timeout = 0;
  priv->lead_user_timer = NULL;
  priv->lead_user = false;
}

void
ghid_set_lock_effects (hidGC gc, AnyObjectType *object)
{
  // XXX: Workaround to crashing exporters
  if (gc->hid != &ghid_hid)
    return;

  /* Only apply effects to locked objects when in "lock" mode */
  if (Settings.Mode == LOCK_MODE &&
      TEST_FLAG (LOCKFLAG, object))
    {
      ghid_set_alpha_mult (gc, 0.8);
      ghid_set_saturation (gc, 0.3);
      ghid_set_brightness (gc, 0.7);
    }
  else
    {
      ghid_set_alpha_mult (gc, 1.0);
      ghid_set_saturation (gc, 1.0);
      ghid_set_brightness (gc, 1.0);
    }
}
