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
#include "gui-pinout-preview.h"

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

#include <gtk/gtkgl.h>
#include "hid/common/hidgl.h"

#include "hid/common/draw_helpers.h"
#include "hid/common/trackball.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

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
static int global_view_2d = 1;

typedef struct render_priv {
  GdkGLConfig *glconfig;
  bool trans_lines;
  bool in_context;
  int subcomposite_stencil_bit;
  char *current_colorname;
  double current_alpha_mult;
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
  Coord width;
  gint cap, join;
} *gtkGC;

static void draw_lead_user (hidGC gc, render_priv *priv);
static void ghid_unproject_to_z_plane (int ex, int ey, Coord pcb_z, Coord *pcb_x, Coord *pcb_y);


#define BOARD_THICKNESS         MM_TO_COORD(1.60)
#define MASK_COPPER_SPACING     MM_TO_COORD(0.05)
#define SILK_MASK_SPACING       MM_TO_COORD(0.01)
static int
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

  top_group = GetLayerGroupNumberBySide (TOP_SIDE);
  bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);

  min_copper_group = MIN (bottom_group, top_group);
  max_copper_group = MAX (bottom_group, top_group);
  num_copper_groups = max_copper_group - min_copper_group + 1;
  middle_copper_group = min_copper_group + num_copper_groups / 2;

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

  end_subcomposite ();

  if (group_visible && subcomposite)
    start_subcomposite ();

  /* Drawing is already flushed by {start,end}_subcomposite */
  hidgl_set_depth (compute_depth (group));

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

static void
ghid_draw_bg_image (void)
{
  static GLuint texture_handle = 0;

  if (!ghidgui->bg_pixbuf)
    return;

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
      glEnable (GL_STENCIL_TEST);                           /* Enable Stencil test */
      stencil_bit = hidgl_assign_clear_stencil_bit (hidgl); /* Get a new (clean) bitplane to stencil with */
      glStencilFunc (GL_ALWAYS, stencil_bit, stencil_bit);  /* Always pass stencil test, write stencil_bit */
      glStencilMask (stencil_bit);                          /* Only write to our subcompositing stencil bitplane */
      glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);           /* Stencil pass => replace stencil value (with 1) */
      break;

    case HID_MASK_AFTER:
      /* Drawing operations as masked to areas where the stencil buffer is '0' */
      glColorMask (1, 1, 1, 1);                             /* Enable drawing of r, g, b & a */
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

  if (priv->current_colorname != NULL &&
      strcmp (priv->current_colorname, gtk_gc->colorname) == 0 &&
      priv->current_alpha_mult == gtk_gc->alpha_mult)
    return;

  free (priv->current_colorname);
  priv->current_colorname = strdup (gtk_gc->colorname);
  priv->current_alpha_mult = gtk_gc->alpha_mult;

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

  if(!priv->in_context)
    return;

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

  hidgl_fill_circle (gc, cx, cy, radius, gport->view.coord_per_px);
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

  hidgl_fill_pcb_polygon (gc, poly, clip_box, gport->view.coord_per_px);
}

void
ghid_thindraw_pcb_polygon (hidGC gc, PolygonType *poly, const BoxType *clip_box)
{
  common_thindraw_pcb_polygon (gc, poly, clip_box);
  ghid_set_alpha_mult (gc, 0.25);
  hid_draw_fill_pcb_polygon (gc, poly, clip_box);
  ghid_set_alpha_mult (gc, 1.0);
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

#define MAX_ELAPSED (50. / 1000.) /* 50ms */
void
ghid_invalidate_all ()
{
  render_priv *priv = gport->render_priv;
  double elapsed = g_timer_elapsed (priv->time_since_expose, NULL);

  ghid_draw_area_update (gport, NULL);

  if (elapsed > MAX_ELAPSED)
    gdk_window_process_all_updates ();
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

void
ghid_init_renderer (int *argc, char ***argv, GHidPort *port)
{
  render_priv *priv;

  port->render_priv = priv = g_new0 (render_priv, 1);

  priv->time_since_expose = g_timer_new ();

  gtk_gl_init(argc, argv);

  /* setup GL-context */
  priv->glconfig = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGBA    |
                                              GDK_GL_MODE_STENCIL |
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

  gtk_widget_set_gl_capability (widget,
                                priv->glconfig,
                                NULL,
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
  draw_pin ((PinType *)b, TEST_FLAG (THINDRAWFLAG, PCB));
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

static int
via_callback (const BoxType * b, void *cl)
{
  draw_via ((PinType *)b, TEST_FLAG (THINDRAWFLAG, PCB));
  return 1;
}

static int
via_inlayer_callback (const BoxType * b, void *cl)
{
  set_pv_inlayer_color ((PinType *) b, cl, VIA_TYPE);
  _draw_pv ((PinType *) b, TEST_FLAG (THINDRAWFLAG, PCB));
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

  if (ON_SIDE (pad, *side))
    draw_pad (pad);
  return 1;
}


static int
hole_callback (const BoxType * b, void *cl)
{
  PinType *pv = (PinType *) b;
  int plated = cl ? *(int *) cl : -1;

  if ((plated == 0 && !TEST_FLAG (HOLEFLAG, pv)) ||
      (plated == 1 &&  TEST_FLAG (HOLEFLAG, pv)))
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

  set_layer_object_color (layer, (AnyObjectType *) line);
  hid_draw_pcb_line (Output.fgGC, line);
  return 1;
}

static int
arc_callback (const BoxType * b, void *cl)
{
  LayerType *layer = cl;
  ArcType *arc = (ArcType *)b;

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
  PolygonType *polygon = (PolygonType *)b;

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
GhidDrawMask (int side, BoxType * screen)
{
  int thin = TEST_FLAG(THINDRAWFLAG, PCB) || TEST_FLAG(THINDRAWPOLYFLAG, PCB);

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
  r_search (PCB->Data->pin_tree, screen, NULL, clearPin_callback_solid, NULL);
  r_search (PCB->Data->via_tree, screen, NULL, clearPin_callback_solid, NULL);
  r_search (PCB->Data->pad_tree, screen, NULL, clearPad_callback_solid, &side);

  hid_draw_use_mask (&ghid_graphics, HID_MASK_AFTER);
  hid_draw_set_color (out->fgGC, PCB->MaskColor);
  ghid_set_alpha_mult (out->fgGC, thin ? 0.35 : 1.0);
  hid_draw_fill_rect (out->fgGC, 0, 0, PCB->MaxWidth, PCB->MaxHeight);
  ghid_set_alpha_mult (out->fgGC, 1.0);

  hid_draw_use_mask (&ghid_graphics, HID_MASK_OFF);
}

static int
GhidDrawLayerGroup (int group, const BoxType * screen)
{
  int i, rv = 1;
  int layernum;
  int side;
  struct poly_info info;
  LayerType *Layer;
  int n_entries = PCB->LayerGroups.Number[group];
  Cardinal *layers = PCB->LayerGroups.Entries[group];
  int first_run = 1;
  int top_group = GetLayerGroupNumberBySide (TOP_SIDE);
  int bottom_group = GetLayerGroupNumberBySide (BOTTOM_SIDE);

  if (!hid_draw_set_layer (&ghid_graphics, 0, group, 0))
    return 0;

  /* HACK: Subcomposite each layer in a layer group separately */
  for (i = n_entries - 1; i >= 0; i--) {
    layernum = layers[i];
    Layer = PCB->Data->Layer + layers[i];

    if (strcmp (Layer->Name, "outline") == 0 ||
        strcmp (Layer->Name, "route") == 0)
      rv = 0;

    if (layernum < max_copper_layer && Layer->On) {

      if (!first_run)
        hid_draw_set_layer (&ghid_graphics, 0, group, 0);

      first_run = 0;

      if (rv && !TEST_FLAG (THINDRAWFLAG, PCB)) {
        /* Mask out drilled holes on this layer */
        hidgl_flush_triangles (&buffer);
        glPushAttrib (GL_COLOR_BUFFER_BIT);
        glColorMask (0, 0, 0, 0);
        hid_draw_set_color (Output.bgGC, PCB->MaskColor);
        if (PCB->PinOn) r_search (PCB->Data->pin_tree, screen, NULL, hole_callback, NULL);
        if (PCB->ViaOn) r_search (PCB->Data->via_tree, screen, NULL, hole_callback, NULL);
        hidgl_flush_triangles (&buffer);
        glPopAttrib ();
      }

      /* draw all polygons on this layer */
      if (Layer->PolygonN) {
        info.layer = Layer;
        info.drawn_area = screen;
        r_search (Layer->polygon_tree, screen, NULL, poly_callback, &info);

        /* HACK: Subcomposite polygons separately from other layer primitives */
        /* Reset the compositing */
        hid_draw_end_layer (&ghid_graphics);
        hid_draw_set_layer (&ghid_graphics, 0, group, 0);

        if (rv && !TEST_FLAG (THINDRAWFLAG, PCB)) {
          hidgl_flush_triangles (&buffer);
          glPushAttrib (GL_COLOR_BUFFER_BIT);
          glColorMask (0, 0, 0, 0);
          /* Mask out drilled holes on this layer */
          if (PCB->PinOn) r_search (PCB->Data->pin_tree, screen, NULL, hole_callback, NULL);
          if (PCB->ViaOn) r_search (PCB->Data->via_tree, screen, NULL, hole_callback, NULL);
          hidgl_flush_triangles (&buffer);
          glPopAttrib ();
        }
      }

      /* Draw pins, vias and pads on this layer */
      if (!global_view_2d && rv) {
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
DrawDrillChannel (int vx, int vy, int vr, int from_layer, int to_layer, double scale)
{
#define PIXELS_PER_CIRCLINE 5.
#define MIN_FACES_PER_CYL 6
#define MAX_FACES_PER_CYL 360
  float radius = vr;
  float x1, y1;
  float x2, y2;
  float z1, z2;
  int i;
  int slices;

  slices = M_PI * 2 * vr / scale / PIXELS_PER_CIRCLINE;

  if (slices < MIN_FACES_PER_CYL)
    slices = MIN_FACES_PER_CYL;

  if (slices > MAX_FACES_PER_CYL)
    slices = MAX_FACES_PER_CYL;

  z1 = compute_depth (from_layer);
  z2 = compute_depth (to_layer);

  x1 = vx + vr;
  y1 = vy;

  hidgl_ensure_triangle_space (&buffer, 2 * slices);
  for (i = 0; i < slices; i++)
    {
      x2 = radius * cosf (((float)(i + 1)) * 2. * M_PI / (float)slices) + vx;
      y2 = radius * sinf (((float)(i + 1)) * 2. * M_PI / (float)slices) + vy;
      hidgl_add_triangle_3D (&buffer, x1, y1, z1,  x2, y2, z1,  x1, y1, z2);
      hidgl_add_triangle_3D (&buffer, x2, y2, z1,  x1, y1, z2,  x2, y2, z2);
      x1 = x2;
      y1 = y2;
    }
}

struct cyl_info {
  int from_layer;
  int to_layer;
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

  hid_draw_set_color (Output.fgGC, color);
  DrawDrillChannel (Pin->X, Pin->Y, Pin->DrillingHole / 2, info->from_layer, info->to_layer, info->scale);
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
  return draw_hole_cyl ((PinType *)b, (struct cyl_info *)cl, VIA_TYPE);
}

void
ghid_draw_everything (BoxType *drawn_area)
{
  render_priv *priv = gport->render_priv;
  int i, ngroups;
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
    DrawSilk (side, drawn_area);

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
  for (i = ngroups - 1; i >= 0; i--) {
    GhidDrawLayerGroup (drawn_groups [i], drawn_area);

#if 1
    if (!global_view_2d && i > 0 &&
        drawn_groups[i] >= min_phys_group &&
        drawn_groups[i] <= max_phys_group &&
        drawn_groups[i - 1] >= min_phys_group &&
        drawn_groups[i - 1] <= max_phys_group) {
      cyl_info.from_layer = drawn_groups[i];
      cyl_info.to_layer = drawn_groups[i - 1];
      cyl_info.scale = gport->view.coord_per_px;
      hid_draw_set_color (Output.fgGC, "drill");
      ghid_set_alpha_mult (Output.fgGC, 0.75);
      if (PCB->PinOn) r_search (PCB->Data->pin_tree, drawn_area, NULL, pin_hole_cyl_callback, &cyl_info);
      if (PCB->ViaOn) r_search (PCB->Data->via_tree, drawn_area, NULL, via_hole_cyl_callback, &cyl_info);
      ghid_set_alpha_mult (Output.fgGC, 1.0);
    }
#endif
  }

  if (TEST_FLAG (CHECKPLANESFLAG, PCB))
    return;

  side = SWAP_IDENT ? BOTTOM_SIDE : TOP_SIDE;

  /* Draw pins, pads, vias below silk */
  if (global_view_2d) {
    start_subcomposite ();

    if (!TEST_FLAG (THINDRAWFLAG, PCB)) {
      /* Mask out drilled holes */
      hidgl_flush_triangles (&buffer);
      glPushAttrib (GL_COLOR_BUFFER_BIT);
      glColorMask (0, 0, 0, 0);
      if (PCB->PinOn) r_search (PCB->Data->pin_tree, drawn_area, NULL, hole_callback, NULL);
      if (PCB->ViaOn) r_search (PCB->Data->via_tree, drawn_area, NULL, hole_callback, NULL);
      hidgl_flush_triangles (&buffer);
      glPopAttrib ();
    }

    if (PCB->PinOn) r_search (PCB->Data->pad_tree, drawn_area, NULL, pad_callback, &side);
    if (PCB->PinOn) r_search (PCB->Data->pin_tree, drawn_area, NULL, pin_callback, NULL);
    if (PCB->ViaOn) r_search (PCB->Data->via_tree, drawn_area, NULL, via_callback, NULL);

    end_subcomposite ();
  }

  /* Draw the solder mask if turned on */
  if (hid_draw_set_layer (&ghid_graphics, SWAP_IDENT ? "soldermask" : "componentmask",
                      SWAP_IDENT ? SL (MASK, BOTTOM) : SL (MASK, TOP), 0)) {
    GhidDrawMask (side, drawn_area);
    hid_draw_end_layer (&ghid_graphics);
  }

  if (hid_draw_set_layer (&ghid_graphics, SWAP_IDENT ? "bottomsilk" : "topsilk",
                      SWAP_IDENT ? SL (SILK, BOTTOM) : SL (SILK, TOP), 0)) {
      DrawSilk (side, drawn_area);
      hid_draw_end_layer (&ghid_graphics);
  }

  /* Draw element Marks */
  if (PCB->PinOn)
    r_search (PCB->Data->element_tree, drawn_area, NULL, EMark_callback, NULL);

  /* Draw rat lines on top */
  if (PCB->RatOn && hid_draw_set_layer (&ghid_graphics, "rats", SL (RATS, 0), 0)) {
    DrawRats(drawn_area);
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
  Coord min_x, min_y;
  Coord max_x, max_y;
  Coord new_x, new_y;
  Coord min_depth;
  Coord max_depth;

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
  glOrtho (0, allocation.width, allocation.height, 0, -100000, 100000);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  glTranslatef (widget->allocation.width / 2., widget->allocation.height / 2., 0);
  glMultMatrixf ((GLfloat *)view_matrix);
  glTranslatef (-widget->allocation.width / 2., -widget->allocation.height / 2., 0);
  glScalef ((port->view.flip_x ? -1. : 1.) / port->view.coord_per_px,
            (port->view.flip_y ? -1. : 1.) / port->view.coord_per_px,
            ((port->view.flip_x == port->view.flip_y) ? 1. : -1.) / port->view.coord_per_px);
  glTranslatef (port->view.flip_x ?  port->view.x0 - PCB->MaxWidth  :
                                    -port->view.x0,
                port->view.flip_y ?  port->view.y0 - PCB->MaxHeight :
                                    -port->view.y0, 0);
  glGetFloatv (GL_MODELVIEW_MATRIX, (GLfloat *)last_modelview_matrix);

  glEnable (GL_STENCIL_TEST);
  glClearColor (port->offlimits_color.red / 65535.,
                port->offlimits_color.green / 65535.,
                port->offlimits_color.blue / 65535.,
                1.);
  glStencilMask (~0);
  glClearStencil (0);
  glClear (GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  hidgl_reset_stencil_usage (priv->hidgl);

  /* Disable the stencil test until we need it - otherwise it gets dirty */
  glDisable (GL_STENCIL_TEST);
  glStencilMask (0);
  glStencilFunc (GL_ALWAYS, 0, 0);

  /* Test the 8 corners of a cube spanning the event */
  min_depth = -50 + compute_depth (0);                    /* FIXME: NEED TO USE PHYSICAL GROUPS */
  max_depth =  50 + compute_depth (max_copper_layer - 1); /* FIXME: NEED TO USE PHYSICAL GROUPS */

  ghid_unproject_to_z_plane (ev->area.x,
                             ev->area.y,
                             min_depth, &new_x, &new_y);
  max_x = min_x = new_x;
  max_y = min_y = new_y;

  ghid_unproject_to_z_plane (ev->area.x,
                             ev->area.y,
                             max_depth, &new_x, &new_y);
  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  /* */
  ghid_unproject_to_z_plane (ev->area.x + ev->area.width,
                             ev->area.y,
                             min_depth, &new_x, &new_y);
  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  ghid_unproject_to_z_plane (ev->area.x + ev->area.width, ev->area.y,
                             max_depth, &new_x, &new_y);
  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  /* */
  ghid_unproject_to_z_plane (ev->area.x + ev->area.width,
                             ev->area.y + ev->area.height,
                             min_depth, &new_x, &new_y);
  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  ghid_unproject_to_z_plane (ev->area.x + ev->area.width,
                             ev->area.y + ev->area.height,
                             max_depth, &new_x, &new_y);
  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  /* */
  ghid_unproject_to_z_plane (ev->area.x,
                             ev->area.y + ev->area.height,
                             min_depth,
                             &new_x, &new_y);
  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

  ghid_unproject_to_z_plane (ev->area.x,
                             ev->area.y + ev->area.height,
                             max_depth,
                             &new_x, &new_y);
  min_x = MIN (min_x, new_x);  max_x = MAX (max_x, new_x);
  min_y = MIN (min_y, new_y);  max_y = MAX (max_y, new_y);

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

  ghid_draw_bg_image ();

  /* hid_expose_callback (&ghid_graphics, &region, 0); */
  ghid_draw_everything (&region);
  hidgl_flush_triangles (priv->hidgl);

  /* Set the current depth to the right value for the layer we are editing */
  priv->edit_depth = compute_depth (GetLayerGroupNumberByNumber (INDEXOFCURRENT));
  hidgl_set_depth (Output.fgGC, priv->edit_depth);

  ghid_draw_grid (Output.fgGC, &region);

  ghid_invalidate_current_gc ();

  DrawAttached (Output.fgGC);
  DrawMark (Output.fgGC);
  hidgl_flush_triangles (priv->hidgl);

  draw_crosshair (Output.fgGC, priv);
  hidgl_flush_triangles (priv->hidgl);

  draw_lead_user (priv->crosshair_gc, priv);

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
  glClear (GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
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

  glconfig = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGB     |
                                        GDK_GL_MODE_STENCIL |
                                        GDK_GL_MODE_SINGLE);

  pixmap = gdk_pixmap_new (NULL, width, height, depth);
  glpixmap = gdk_pixmap_set_gl_capability (pixmap, glconfig, NULL);
  gldrawable = GDK_GL_DRAWABLE (glpixmap);
  glcontext = gdk_gl_context_new (gldrawable, NULL, TRUE, GDK_GL_RGBA_TYPE);

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
  glClear (GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
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

static float
determinant_2x2 (float m[2][2])
{
  float det;
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
invert_2x2 (float m[2][2], float out[2][2])
{
  float scale = 1 / determinant_2x2 (m);
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


static void
ghid_unproject_to_z_plane (int ex, int ey, Coord pcb_z, Coord *pcb_x, Coord *pcb_y)
{
  float mat[2][2];
  float inv_mat[2][2];
  float x, y;

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

    ex - view_matrix[0][3] * 1
       - view_matrix[0][2] * vz
      = view_matrix[0][0] * vx +
        view_matrix[0][1] * vy;

    ey - view_matrix[1][3] * 1
       - view_matrix[1][2] * vz
      = view_matrix[1][0] * vx +
        view_matrix[1][1] * vy;
  */

  /* NB: last_modelview_matrix is transposed in memory! */
  x = (float)ex - last_modelview_matrix[3][0] * 1
                - last_modelview_matrix[2][0] * pcb_z;

  y = (float)ey - last_modelview_matrix[3][1] * 1
                - last_modelview_matrix[2][1] * pcb_z;

  /*
    x = view_matrix[0][0] * vx +
        view_matrix[0][1] * vy;

    y = view_matrix[1][0] * vx +
        view_matrix[1][1] * vy;

    [view_matrix[0][0] view_matrix[0][1]] [vx] = [x]
    [view_matrix[1][0] view_matrix[1][1]] [vy]   [y]
  */

  mat[0][0] = last_modelview_matrix[0][0];
  mat[0][1] = last_modelview_matrix[1][0];
  mat[1][0] = last_modelview_matrix[0][1];
  mat[1][1] = last_modelview_matrix[1][1];

  /*    if (determinant_2x2 (mat) < 0.00001)       */
  /*      printf ("Determinant is quite small\n"); */

  invert_2x2 (mat, inv_mat);

  *pcb_x = (int)(inv_mat[0][0] * x + inv_mat[0][1] * y);
  *pcb_y = (int)(inv_mat[1][0] * x + inv_mat[1][1] * y);
}


bool
ghid_event_to_pcb_coords (int event_x, int event_y, Coord *pcb_x, Coord *pcb_y)
{
  render_priv *priv = gport->render_priv;

  ghid_unproject_to_z_plane (event_x, event_y, priv->edit_depth, pcb_x, pcb_y);

  return true;
}

bool
ghid_pcb_to_event_coords (Coord pcb_x, Coord pcb_y, int *event_x, int *event_y)
{
  render_priv *priv = gport->render_priv;

  /* NB: last_modelview_matrix is transposed in memory */
  float w = last_modelview_matrix[0][3] * (float)pcb_x +
            last_modelview_matrix[1][3] * (float)pcb_y +
            last_modelview_matrix[2][3] * 0. +
            last_modelview_matrix[3][3] * 1.;

  *event_x = (last_modelview_matrix[0][0] * (float)pcb_x +
              last_modelview_matrix[1][0] * (float)pcb_y +
              last_modelview_matrix[2][0] * priv->edit_depth +
              last_modelview_matrix[3][0] * 1.) / w;
  *event_y = (last_modelview_matrix[0][1] * (float)pcb_x +
              last_modelview_matrix[1][1] * (float)pcb_y +
              last_modelview_matrix[2][1] * priv->edit_depth +
              last_modelview_matrix[3][1] * 1.) / w;

  return true;
}

void
ghid_view_2d (void *ball, gboolean view_2d, gpointer userdata)
{
  global_view_2d = view_2d;
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
